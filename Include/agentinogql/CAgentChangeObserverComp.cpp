// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CAgentChangeObserverComp.h"


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


namespace
{


QString StatusWireString(agentinodata::IServiceStatusInfo::ServiceStatus status)
{
	// Same wire form as CServiceStatusCollectionSubscriberControllerComp / QML normalizeServiceStatus.
	switch (status){
	case agentinodata::IServiceStatusInfo::SS_RUNNING:
		return QStringLiteral("RUNNING");
	case agentinodata::IServiceStatusInfo::SS_NOT_RUNNING:
		return QStringLiteral("NOT_RUNNING");
	case agentinodata::IServiceStatusInfo::SS_STARTING:
		return QStringLiteral("STARTING");
	case agentinodata::IServiceStatusInfo::SS_STOPPING:
		return QStringLiteral("STOPPING");
	case agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
		return QStringLiteral("RUNNING_IMPOSSIBLE");
	default:
		return QStringLiteral("UNDEFINED");
	}
}


} // namespace


// protected methods

// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CAgentChangeObserverComp::OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData)
{
	// WebSocket RequestClientHandler delivers agent pushes on the socket I/O thread.
	// StartService runs on a GQL worker with a nested event loop; mutating collections
	// or re-registering subscriptions from that I/O thread races those workers and can
	// leave the server unable to handle further GUI commands until reconnect.
	QueueHandleResponse(subscriptionId, subscriptionData);
}


void CAgentChangeObserverComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


// private methods

void CAgentChangeObserverComp::QueueHandleResponse(
			const QByteArray& subscriptionId,
			const QByteArray& subscriptionData)
{
	if (QThread::currentThread() == thread()){
		HandleResponseOnOwnerThread(subscriptionId, subscriptionData);

		return;
	}

	const bool queued = QMetaObject::invokeMethod(
				this,
				[this, subscriptionId, subscriptionData](){
					HandleResponseOnOwnerThread(subscriptionId, subscriptionData);
				},
				Qt::QueuedConnection);

	if (!queued){
		SendErrorMessage(
					0,
					QString("Unable to queue live subscription payload (id=%1)")
								.arg(QString::fromUtf8(subscriptionId)),
					"CAgentChangeObserverComp");
	}
}


void CAgentChangeObserverComp::HandleResponseOnOwnerThread(
			const QByteArray& subscriptionId,
			const QByteArray& subscriptionData)
{
	if (m_registeredServiceCollectionAgents.values().contains(subscriptionId)){
		HandleServiceCollectionChanged(subscriptionId, subscriptionData);

		return;
	}

	if (m_registeredAgents.values().contains(subscriptionId)){
		HandleServiceStatusChanged(subscriptionId, subscriptionData);
	}
}

QJsonObject CAgentChangeObserverComp::ExtractStatusPayload(const QJsonObject& root)
{
	QJsonObject payload = root.value(QStringLiteral("OnAgentServiceStatusChanged")).toObject();
	if (!payload.isEmpty()){
		return payload;
	}

	// Already-unwrapped body (publisher may pass the inner object only).
	if (root.contains(QStringLiteral("serviceid"))
		|| root.contains(QStringLiteral("serviceStatus"))
		|| root.contains(QStringLiteral("status"))
		|| root.contains(QStringLiteral("id"))){
		return root;
	}

	// AWS-style nesting: payload.data.OnAgentServiceStatusChanged
	payload = root.value(QStringLiteral("data")).toObject().value(QStringLiteral("OnAgentServiceStatusChanged")).toObject();
	if (!payload.isEmpty()){
		return payload;
	}

	return QJsonObject();
}


bool CAgentChangeObserverComp::ParseServiceStatus(
			const QString& statusText,
			agentinodata::IServiceStatusInfo::ServiceStatus& status)
{
	// Single shared parser: the agent publishes status as I_DECLARE_ENUM names
	// (EmitChangeSignal), as ProcessStateEnum ids (GetService / ListObjects) and as the
	// uppercase GQL wire form. Both the live push path here and the reconcile path in
	// CServiceControllerProxyComp must interpret them identically.
	return agentinodata::GetServiceStatusFromRepresentation(statusText, status);
}


CAgentChangeObserverComp::ApplyStatusResult CAgentChangeObserverComp::ApplyServiceStatus(
			const QByteArray& serviceId,
			agentinodata::IServiceStatusInfo::ServiceStatus status)
{
	if (!m_serviceStatusCollectionCompPtr.IsValid() || serviceId.isEmpty()){
		return ApplyStatusResult::Failed;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		agentinodata::CServiceStatusInfo* serviceStatusInfoPtr =
					dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
		if (serviceStatusInfoPtr == nullptr){
			return ApplyStatusResult::Failed;
		}

		// No collection write: proxy may already have written the same status (e.g. STARTING).
		// Caller still notifies ServiceStatusSubscriberController so GUI gets the agent push.
		if (serviceStatusInfoPtr->GetServiceStatus() == status){
			return ApplyStatusResult::Unchanged;
		}

		serviceStatusInfoPtr->SetServiceStatus(status);

		if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr)){
			return ApplyStatusResult::Failed;
		}

		return ApplyStatusResult::Changed;
	}

	istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
	serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);
	serviceStatusInfoPtr->SetServiceId(serviceId);
	serviceStatusInfoPtr->SetServiceStatus(status);

	const QByteArray insertedId = m_serviceStatusCollectionCompPtr->InsertNewObject(
				"ServiceStatusInfo",
				"",
				"",
				serviceStatusInfoPtr.PopPtr(),
				serviceId);

	return insertedId.isEmpty() ? ApplyStatusResult::Failed : ApplyStatusResult::Changed;
}


bool CAgentChangeObserverComp::IsAgentIngestionAllowed(const QByteArray& agentId) const
{
	if (agentId.isEmpty()){
		return false;
	}
	// No gate wired → preserve open behavior (tests / stripped shells).
	if (!m_enrollmentControllerCompPtr.IsValid()){
		return true;
	}
	const EnrollmentRecord record = m_enrollmentControllerCompPtr->Get(agentId);
	return record.status == EnrollmentStatus::Approved;
}


void CAgentChangeObserverComp::DropLiveSubscriptionsForAgent(const QByteArray& agentId)
{
	if (agentId.isEmpty()){
		return;
	}
	UnregisterStatusSubscription(agentId);
	UnregisterCollectionSubscription(agentId);
}


void CAgentChangeObserverComp::HandleServiceStatusChanged(
			const QByteArray& subscriptionId,
			const QByteArray& subscriptionData)
{
	// Resolve agentId from live subscription registry (agentId -> subscriptionId).
	QByteArray agentId;
	for (auto it = m_registeredAgents.constBegin(); it != m_registeredAgents.constEnd(); ++it){
		if (it.value() == subscriptionId || subscriptionId.contains(it.key())){
			agentId = it.key();
			break;
		}
	}

	// Live-path enrollment gate: revoke/reject must stop ingestion immediately (not only on reconnect).
	if (!agentId.isEmpty() && !IsAgentIngestionAllowed(agentId)){
		DropLiveSubscriptionsForAgent(agentId);
		return;
	}

	const QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	const QJsonObject payload = ExtractStatusPayload(document.object());
	if (payload.isEmpty()){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: empty/unrecognized payload: %1")
								.arg(QString::fromUtf8(subscriptionData)),
					"CAgentChangeObserverComp");

		return;
	}

	QByteArray serviceId = payload.value(QStringLiteral("serviceid")).toString().toUtf8();
	if (serviceId.isEmpty()){
		serviceId = payload.value(QStringLiteral("id")).toString().toUtf8();
	}
	if (serviceId.isEmpty()){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: missing service id in payload: %1")
								.arg(QString::fromUtf8(subscriptionData)),
					"CAgentChangeObserverComp");

		return;
	}

	QString statusText = payload.value(agentino::ServiceStatus::s_Key).toString();
	if (statusText.isEmpty()){
		statusText = payload.value(QStringLiteral("status")).toString();
	}

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus =
				agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	if (!ParseServiceStatus(statusText, serviceStatus)){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: unknown status '%1' for service '%2'")
								.arg(statusText, QString::fromUtf8(serviceId)),
					"CAgentChangeObserverComp");

		return;
	}

	// The agent is the single source of truth for status; the server only projects
	// the notification into the GUI-facing ServiceStatusCollection.
	// ApplyServiceStatus skips identical writes (no collection churn).
	const ApplyStatusResult applyResult = ApplyServiceStatus(serviceId, serviceStatus);
	if (applyResult == ApplyStatusResult::Failed){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: unable to apply status '%1' for service '%2' into collection")
								.arg(StatusWireString(serviceStatus), QString::fromUtf8(serviceId)),
					"CAgentChangeObserverComp");
	}

	if (applyResult == ApplyStatusResult::Changed){
		// ServiceStatusCollection just changed - MI_SERVICE_STATUS_COLLECTION
		// (OnServiceStatusCollectionChanged, registered on m_serviceStatusModelCompPtr)
		// observes that write directly and relays it to GUI clients. Do NOT also emit
		// here: that used to be the only relay, and returning early exactly on this
		// branch (the case that matters!) is what silently swallowed every real status
		// transition - see the removed "CollectionSubscriber already fans out" comment
		// that described a subscriber wiring which never actually existed.
		return;
	}

	// Unchanged (or Failed to write): no collection write happened, so nothing will
	// trigger OnServiceStatusCollectionChanged. Relay the agent's report directly so an
	// identical-status heartbeat / a failed mirror write still reaches the GUI.
	EmitServiceStatusNotification(serviceId, serviceStatus);
}


void CAgentChangeObserverComp::EmitServiceStatusNotification(
			const QByteArray& serviceId,
			agentinodata::IServiceStatusInfo::ServiceStatus status)
{
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	notifierInfo.serviceId = serviceId;
	notifierInfo.serviceStatus = status;
	changeSet.SetChangeInfo(
				agentinodata::IServiceController::CN_STATUS_CHANGED,
				QVariant::fromValue(notifierInfo));
	changeSet.SetChangeInfo(QByteArrayLiteral("serviceid"), serviceId);
	istd::CChangeNotifier notifier(this, &changeSet);
}


void CAgentChangeObserverComp::OnServiceStatusCollectionChanged(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		return;
	}

	QByteArray serviceId;
	if (changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED)){
		serviceId = changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_INSERTED).toByteArray();
	}
	else if (changeSet.Contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_OBJECT_DATA_CHANGED).toByteArray();
	}

	if (serviceId.isEmpty()){
		// Removal / unrelated change (CF_REMOVED etc.) - nothing to relay as a status.
		return;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		return;
	}

	agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
	if (serviceStatusInfoPtr == nullptr){
		return;
	}

	EmitServiceStatusNotification(serviceId, serviceStatusInfoPtr->GetServiceStatus());
}


void CAgentChangeObserverComp::HandleServiceCollectionChanged(const QByteArray& subscriptionId, const QByteArray& subscriptionData)
{
	if (!m_serviceSynchronizerCompPtr.IsValid()){
		SendErrorMessage(0, "ServiceSynchronizer is not set; live collection push ignored", "CAgentChangeObserverComp");

		return;
	}

	// Resolve the agent that owns this subscription.
	QByteArray agentId;
	for (auto it = m_registeredServiceCollectionAgents.constBegin(); it != m_registeredServiceCollectionAgents.constEnd(); ++it){
		if (it.value() == subscriptionId){
			agentId = it.key();

			break;
		}
	}

	if (agentId.isEmpty()){
		SendErrorMessage(0, "Live collection push: unknown subscription id", "CAgentChangeObserverComp");

		return;
	}

	if (!IsAgentIngestionAllowed(agentId)){
		DropLiveSubscriptionsForAgent(agentId);
		return;
	}

	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	QJsonObject root = document.object();
	QJsonObject payload = root.value("OnAgentServicesCollectionChanged").toObject();
	// Fallbacks: already-unwrapped payload, or AWS-style data nesting.
	if (payload.isEmpty() && root.contains(QStringLiteral("typeOperation"))){
		payload = root;
	}
	if (payload.isEmpty()){
		payload = root.value(QStringLiteral("data")).toObject().value("OnAgentServicesCollectionChanged").toObject();
	}
	if (payload.isEmpty()){
		SendErrorMessage(
					0,
					QString("Live collection push from agent '%1': empty/unrecognized payload: %2")
								.arg(QString::fromUtf8(agentId), QString::fromUtf8(subscriptionData)),
					"CAgentChangeObserverComp");

		return;
	}

	QString typeOperation = payload.value("typeOperation").toString();

	QString errorMessage;
	if (typeOperation == "removed"){
		QByteArrayList serviceIds;
		const QJsonArray removedItems = payload.value("itemIds").toArray();
		for (const QJsonValue& value: removedItems){
			serviceIds << value.toString().toUtf8();
		}

		if (!serviceIds.isEmpty()){
			if (!m_serviceSynchronizerCompPtr->RemoveServicesInMirror(agentId, serviceIds, errorMessage)){
				SendErrorMessage(0, errorMessage, "CAgentChangeObserverComp");
			}
		}
	}
	else{
		// "inserted" or "updated" (or missing type): pull current service data from the agent.
		QByteArray serviceId = payload.value("itemId").toString().toUtf8();
		if (serviceId.isEmpty()){
			SendErrorMessage(
						0,
						QString("Live collection push from agent '%1': missing itemId (typeOperation=%2)")
									.arg(QString::fromUtf8(agentId), typeOperation),
						"CAgentChangeObserverComp");

			return;
		}

		if (!m_serviceSynchronizerCompPtr->SyncServiceInMirror(agentId, serviceId, errorMessage)){
			SendErrorMessage(0, errorMessage, "CAgentChangeObserverComp");
		}
	}
}


void CAgentChangeObserverComp::RegisterStatusSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid() || agentId.isEmpty()){
		return;
	}

	imtgql::CGqlRequest gqlAddRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, "OnAgentServiceStatusChanged");
	imtgql::CGqlParamObject subscriptionInput;
	gqlAddRequest.AddParam("input", subscriptionInput);

	// Field names match the agent publish body (CServiceSubscriberControllerComp).
	imtgql::CGqlFieldObject subscriptionField;
	subscriptionField.InsertField("serviceid");
	subscriptionField.InsertField("serviceStatus");
	gqlAddRequest.AddField("data", subscriptionField);

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlAddRequest.SetGqlContext(gqlContextPtr);

	QByteArray subscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlAddRequest, this);
	if (!subscriptionId.isEmpty()){
		m_registeredAgents.insert(agentId, subscriptionId);
	}
	else{
		SendErrorMessage(
					0,
					QString("Unable to register OnAgentServiceStatusChanged for agent '%1'")
								.arg(QString::fromUtf8(agentId)),
					"CAgentChangeObserverComp");
	}
}


void CAgentChangeObserverComp::RegisterCollectionSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid() || !m_serviceSynchronizerCompPtr.IsValid() || agentId.isEmpty()){
		return;
	}

	imtgql::CGqlRequest gqlCollectionRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, "OnAgentServicesCollectionChanged");
	imtgql::CGqlParamObject collectionInput;
	gqlCollectionRequest.AddParam("input", collectionInput);

	imtgql::CGqlFieldObject collectionField;
	collectionField.InsertField("typeOperation");
	collectionField.InsertField("itemId");
	collectionField.InsertField("itemIds");
	gqlCollectionRequest.AddField("data", collectionField);

	imtgql::CGqlContext* collectionContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers collectionHeaders;
	collectionHeaders.insert("clientid", agentId);
	collectionContextPtr->SetHeaders(collectionHeaders);
	gqlCollectionRequest.SetGqlContext(collectionContextPtr);

	QByteArray collectionSubscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlCollectionRequest, this);
	if (!collectionSubscriptionId.isEmpty()){
		m_registeredServiceCollectionAgents.insert(agentId, collectionSubscriptionId);
	}
	else{
		SendErrorMessage(
					0,
					QString("Unable to register OnAgentServicesCollectionChanged for agent '%1'")
								.arg(QString::fromUtf8(agentId)),
					"CAgentChangeObserverComp");
	}
}


void CAgentChangeObserverComp::UnregisterStatusSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return;
	}

	if (m_registeredAgents.contains(agentId)){
		m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredAgents[agentId]);
		m_registeredAgents.remove(agentId);
	}
}


void CAgentChangeObserverComp::UnregisterCollectionSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return;
	}

	if (m_registeredServiceCollectionAgents.contains(agentId)){
		m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredServiceCollectionAgents[agentId]);
		m_registeredServiceCollectionAgents.remove(agentId);
	}
}


void CAgentChangeObserverComp::EnsureLiveSubscriptionsForAgent(const QByteArray& agentId, bool forceReregister)
{
	if (agentId.isEmpty()){
		return;
	}

	// Quarantined / revoked / pending: no domain live ingestion.
	if (!IsAgentIngestionAllowed(agentId)){
		DropLiveSubscriptionsForAgent(agentId);
		return;
	}

	if (forceReregister && m_registeredAgents.contains(agentId)){
		// Drop stale WS registration on the agent (e.g. after reconnect) and open a fresh one.
		// m_registeredAgents is deliberately NOT cleared on AS_DISCONNECTED (see OnModelChanged
		// below), while the agent's own CGqlPublisherCompBase subscriber list IS wiped the
		// instant its socket dies - so without this, the "already registered" check right below
		// would keep skipping resubscription forever after any reconnect, silently dropping
		// every OnAgentServiceStatusChanged push from then on (StartService/StopService would
		// still run fine on the agent, but the server would never hear about it).
		UnregisterStatusSubscription(agentId);
	}

	if (!m_registeredAgents.contains(agentId)){
		RegisterStatusSubscription(agentId);
	}

	if (!m_serviceSynchronizerCompPtr.IsValid()){
		return;
	}

	if (forceReregister && m_registeredServiceCollectionAgents.contains(agentId)){
		// Drop stale WS registration on the agent (e.g. after reconnect) and open a fresh one.
		UnregisterCollectionSubscription(agentId);
	}

	if (!m_registeredServiceCollectionAgents.contains(agentId)){
		RegisterCollectionSubscription(agentId);
	}
}


void CAgentChangeObserverComp::RefreshSubscriptionsFromAgentCollection(bool forceReregisterExisting)
{
	if (!m_subscriptionManagerCompPtr.IsValid() || !m_agentCollectionCompPtr.IsValid()){
		return;
	}

	imtbase::ICollectionInfo::Ids agentCollectionIds = m_agentCollectionCompPtr->GetElementIds();

	for (const QByteArray& registeredAgentId: m_registeredAgents.keys()){
		if (!agentCollectionIds.contains(registeredAgentId)){
			UnregisterStatusSubscription(registeredAgentId);
		}
	}

	for (const QByteArray& registeredAgentId: m_registeredServiceCollectionAgents.keys()){
		if (!agentCollectionIds.contains(registeredAgentId)){
			UnregisterCollectionSubscription(registeredAgentId);
		}
	}

	for (const QByteArray& agentId: agentCollectionIds){
		// Membership / AgentAdd noise must NOT force drop+reopen of live subscriptions.
		// Force re-register is reserved for AS_CONNECTED (socket just came back; agent-side
		// subscriber lists were wiped). Unconditional force here raced StartService's nested
		// event loop and left the server unable to process further GUI commands.
		EnsureLiveSubscriptionsForAgent(agentId, forceReregisterExisting);
	}
}


// reimplemented (imod::CMultiModelDispatcherBase)

void CAgentChangeObserverComp::OnModelChanged(int modelId, const istd::IChangeable::ChangeSet& changeSet)
{
	if (modelId == MI_LOGIN_STATUS){
		// Agent WS connected / disconnected — set agent + service status and endpoint
		// availability. SetAgentStatus writes AgentStatusCollection, which re-enters
		// OnModelChanged(MI_AGENT_STATUS_COLLECTION) to (re)register live subscriptions.
		HandleAgentConnectionChanged(changeSet);
		return;
	}

	if (modelId == MI_AGENT_COLLECTION){
		// Only real membership changes — not mirror/service data churn on the same object.
		const bool agentMembershipChange =
					changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED) ||
					changeSet.Contains(imtbase::ICollectionInfo::CF_REMOVED) ||
					changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_RENAMED);

		// CF_OBJECT_DATA_CHANGED / CF_ANY fire often (LastConnection, service mirror) —
		// only fill missing subscriptions, never force re-register.
		const bool maybeMissingSubs =
					agentMembershipChange ||
					changeSet.Contains(istd::IChangeable::CF_ANY) ||
					changeSet.Contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED);

		if (maybeMissingSubs){
			RefreshSubscriptionsFromAgentCollection(false);
		}

		return;
	}

	if (modelId == MI_SERVICE_STATUS_COLLECTION){
		OnServiceStatusCollectionChanged(changeSet);
		return;
	}

	if (modelId == MI_AGENT_STATUS_COLLECTION){
		// Agent went online/offline — re-open live subscriptions while the WS is up.
		if (!m_agentStatusCollectionCompPtr.IsValid()){
			return;
		}

		imtbase::ICollectionInfo::Ids statusIds = m_agentStatusCollectionCompPtr->GetElementIds();
		for (const QByteArray& agentId: statusIds){
			imtbase::IObjectCollection::DataPtr dataPtr;
			if (!m_agentStatusCollectionCompPtr->GetObjectData(agentId, dataPtr)){
				continue;
			}

			agentinodata::CAgentStatusInfo* statusInfoPtr = dynamic_cast<agentinodata::CAgentStatusInfo*>(dataPtr.GetPtr());
			if (statusInfoPtr == nullptr){
				continue;
			}

			if (statusInfoPtr->GetAgentStatus() == agentinodata::IAgentStatusInfo::AS_CONNECTED){
				// Agent socket is up again — agent-side publishers were cleared on disconnect.
				EnsureLiveSubscriptionsForAgent(agentId, true);
				// Also pull ServicesList into the volatile server mirror (Topology / Agents.services).
				// Relying only on AgentCollectionController's deferred timer was easy to miss after
				// reconnect when the agent row already existed.
				if (m_serviceSynchronizerCompPtr.IsValid() && IsAgentIngestionAllowed(agentId)){
					QString reconcileError;
					if (!m_serviceSynchronizerCompPtr->SyncAgentServicesInMirror(agentId, reconcileError)
								&& !reconcileError.isEmpty()){
						SendErrorMessage(
									0,
									QString("Reconnect reconcile for agent '%1' failed: %2")
												.arg(QString::fromUtf8(agentId), reconcileError),
									"CAgentChangeObserverComp");
					}
				}
			}
			else if (statusInfoPtr->GetAgentStatus() == agentinodata::IAgentStatusInfo::AS_DISCONNECTED){
				// Keep map entries so SubscriptionManager can re-start them on CS_CONNECTED;
				// agent-side registered subscribers are already cleared when the socket dies.
			}
		}
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentChangeObserverComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		RegisterModel(m_modelCompPtr.GetPtr(), MI_AGENT_COLLECTION);
	}

	if (m_agentStatusModelCompPtr.IsValid()){
		RegisterModel(m_agentStatusModelCompPtr.GetPtr(), MI_AGENT_STATUS_COLLECTION);
	}

	// Absorbed connection observer: watch agent WS connect/disconnect directly.
	if (m_loginStatusModelCompPtr.IsValid()){
		RegisterModel(m_loginStatusModelCompPtr.GetPtr(), MI_LOGIN_STATUS);
	}

	// Catch-all relay: whichever component wrote ServiceStatusCollection (live push,
	// disconnect reset, reconnect reconcile, Start/Stop proxy), this makes sure GUI
	// subscribers always hear about it - see the ModelId enum / class doc comment.
	if (m_serviceStatusModelCompPtr.IsValid()){
		RegisterModel(m_serviceStatusModelCompPtr.GetPtr(), MI_SERVICE_STATUS_COLLECTION);
	}

	// Agents already present in the mirror (DB load / already connected).
	RefreshSubscriptionsFromAgentCollection(false);
}


void CAgentChangeObserverComp::OnComponentDestroyed()
{
	UnregisterAllModels();

	BaseClass::OnComponentDestroyed();
}


// Absorbed from CAgentConnectionObserverComp

void CAgentChangeObserverComp::HandleAgentConnectionChanged(const istd::IChangeable::ChangeSet& changeSet)
{
	const QByteArray agentId = changeSet.GetChangeInfo("ClientId").toByteArray();
	if (agentId.isEmpty()){
		return;
	}

	if (changeSet.Contains(imtcom::IConnectionStatusProvider::CS_CONNECTED)){
		SetAgentStatus(agentId, agentinodata::IAgentStatusInfo::AS_CONNECTED);
	}
	else{
		ResetAgentServiceStatuses(agentId);
		SetAgentStatus(agentId, agentinodata::IAgentStatusInfo::AS_DISCONNECTED);
	}
}


void CAgentChangeObserverComp::SetAgentStatus(const QByteArray& agentId, agentinodata::IAgentStatusInfo::AgentStatus status)
{
	if (!m_agentStatusCollectionCompPtr.IsValid()){
		return;
	}

	istd::TDelPtr<agentinodata::CAgentStatusInfo> statusInfoPtr;
	statusInfoPtr.SetPtr(new agentinodata::CAgentStatusInfo(agentId, status));

	if (m_agentStatusCollectionCompPtr->GetElementIds().contains(agentId)){
		m_agentStatusCollectionCompPtr->SetObjectData(agentId, *statusInfoPtr.PopPtr());
	}
	else{
		m_agentStatusCollectionCompPtr->InsertNewObject("AgentStatusInfo", "", "", statusInfoPtr.PopPtr(), agentId);
	}
}


void CAgentChangeObserverComp::ResetAgentServiceStatuses(const QByteArray& agentId)
{
	if (!m_serviceManagerCompPtr.IsValid() || !m_agentCollectionCompPtr.IsValid()){
		return;
	}

	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (!m_agentCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
		return;
	}

	imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr){
		return;
	}

	const imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceId : ids){
		ApplyServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
	}
}


} // namespace agentinogql
