// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CSubscriptionControllerComp.h"


// Qt includes
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

void CSubscriptionControllerComp::OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData)
{
	// WebSocket RequestClientHandler delivers agent pushes on the socket I/O thread.
	// StartService runs on a GQL worker with a nested event loop; mutating collections
	// or re-registering subscriptions from that I/O thread races those workers and can
	// leave the server unable to handle further GUI commands until reconnect.
	QueueHandleResponse(subscriptionId, subscriptionData);
}


void CSubscriptionControllerComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


// private methods

void CSubscriptionControllerComp::QueueHandleResponse(
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
					"CSubscriptionControllerComp");
	}
}


void CSubscriptionControllerComp::HandleResponseOnOwnerThread(
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

QJsonObject CSubscriptionControllerComp::ExtractStatusPayload(const QJsonObject& root)
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


bool CSubscriptionControllerComp::ParseServiceStatus(
			const QString& statusText,
			agentinodata::IServiceStatusInfo::ServiceStatus& status)
{
	if (statusText.isEmpty()){
		return false;
	}

	// I_DECLARE_ENUM form used by agent EmitChangeSignal / ServiceSubscriberController.
	if (agentinodata::IServiceStatusInfo::FromString(statusText.toUtf8(), status)){
		return true;
	}

	const QString normalized = statusText.trimmed();
	const QString upper = normalized.toUpper();
	const QString lower = normalized.toLower();

	// Wire form published by CServiceStatusCollectionSubscriberControllerComp / QML.
	if (upper == QStringLiteral("RUNNING") || lower == QStringLiteral("running")){
		status = agentinodata::IServiceStatusInfo::SS_RUNNING;

		return true;
	}
	if (upper == QStringLiteral("NOT_RUNNING") || lower == QStringLiteral("notrunning") || normalized == QStringLiteral("notRunning")){
		status = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;

		return true;
	}
	if (upper == QStringLiteral("STARTING") || lower == QStringLiteral("starting")){
		status = agentinodata::IServiceStatusInfo::SS_STARTING;

		return true;
	}
	if (upper == QStringLiteral("STOPPING") || lower == QStringLiteral("stopping")){
		status = agentinodata::IServiceStatusInfo::SS_STOPPING;

		return true;
	}
	if (upper == QStringLiteral("RUNNING_IMPOSSIBLE") || upper == QStringLiteral("SS_RUNNING_IMPOSSIBLE")){
		status = agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE;

		return true;
	}
	if (upper == QStringLiteral("UNDEFINED") || lower == QStringLiteral("undefined")){
		status = agentinodata::IServiceStatusInfo::SS_UNDEFINED;

		return true;
	}

	return false;
}


CSubscriptionControllerComp::ApplyStatusResult CSubscriptionControllerComp::ApplyServiceStatus(
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


void CSubscriptionControllerComp::HandleServiceStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const QByteArray& subscriptionData)
{
	const QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	const QJsonObject payload = ExtractStatusPayload(document.object());
	if (payload.isEmpty()){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: empty/unrecognized payload: %1")
								.arg(QString::fromUtf8(subscriptionData)),
					"CSubscriptionControllerComp");

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
					"CSubscriptionControllerComp");

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
					"CSubscriptionControllerComp");

		return;
	}

	// 1) Write ServiceStatusCollection when the value actually changes → CollectionSubscriber
	//    publishes OnServiceStatusChanged (with dependencyStatus) for the GUI.
	// 2) If the write was a no-op (proxy already stored the same status) or failed, still
	//    CChangeNotifier so ServiceStatusSubscriberController (Model=this) publishes
	//    OnServiceStatusChanged. Dropping this path was the main regression after the
	//    agent↔server live rewrite: agent STARTING after proxy STARTING never reached the GUI.
	const ApplyStatusResult applyResult = ApplyServiceStatus(serviceId, serviceStatus);
	if (applyResult == ApplyStatusResult::Failed){
		SendErrorMessage(
					0,
					QString("OnAgentServiceStatusChanged: unable to apply status '%1' for service '%2' into collection")
								.arg(StatusWireString(serviceStatus), QString::fromUtf8(serviceId)),
					"CSubscriptionControllerComp");
	}

	if (applyResult == ApplyStatusResult::Changed){
		// CollectionSubscriber already fans out OnServiceStatusChanged.
		return;
	}

	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	notifierInfo.serviceId = serviceId;
	notifierInfo.serviceStatus = serviceStatus;
	changeSet.SetChangeInfo(
				agentinodata::IServiceController::CN_STATUS_CHANGED,
				QVariant::fromValue(notifierInfo));
	changeSet.SetChangeInfo(QByteArrayLiteral("serviceid"), serviceId);
	istd::CChangeNotifier notifier(this, &changeSet);
}


void CSubscriptionControllerComp::HandleServiceCollectionChanged(const QByteArray& subscriptionId, const QByteArray& subscriptionData)
{
	if (!m_serviceSynchronizerCompPtr.IsValid()){
		SendErrorMessage(0, "ServiceSynchronizer is not set; live collection push ignored", "CSubscriptionControllerComp");

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
		SendErrorMessage(0, "Live collection push: unknown subscription id", "CSubscriptionControllerComp");

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
					"CSubscriptionControllerComp");

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
				SendErrorMessage(0, errorMessage, "CSubscriptionControllerComp");
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
						"CSubscriptionControllerComp");

			return;
		}

		if (!m_serviceSynchronizerCompPtr->SyncServiceInMirror(agentId, serviceId, errorMessage)){
			SendErrorMessage(0, errorMessage, "CSubscriptionControllerComp");
		}
	}
}


void CSubscriptionControllerComp::RegisterStatusSubscription(const QByteArray& agentId)
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
					"CSubscriptionControllerComp");
	}
}


void CSubscriptionControllerComp::RegisterCollectionSubscription(const QByteArray& agentId)
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
					"CSubscriptionControllerComp");
	}
}


void CSubscriptionControllerComp::UnregisterStatusSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return;
	}

	if (m_registeredAgents.contains(agentId)){
		m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredAgents[agentId]);
		m_registeredAgents.remove(agentId);
	}
}


void CSubscriptionControllerComp::UnregisterCollectionSubscription(const QByteArray& agentId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return;
	}

	if (m_registeredServiceCollectionAgents.contains(agentId)){
		m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredServiceCollectionAgents[agentId]);
		m_registeredServiceCollectionAgents.remove(agentId);
	}
}


void CSubscriptionControllerComp::EnsureLiveSubscriptionsForAgent(const QByteArray& agentId, bool forceReregister)
{
	if (agentId.isEmpty()){
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


void CSubscriptionControllerComp::RefreshSubscriptionsFromAgentCollection(bool forceReregisterExisting)
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

void CSubscriptionControllerComp::OnModelChanged(int modelId, const istd::IChangeable::ChangeSet& changeSet)
{
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
			}
			else if (statusInfoPtr->GetAgentStatus() == agentinodata::IAgentStatusInfo::AS_DISCONNECTED){
				// Keep map entries so SubscriptionManager can re-start them on CS_CONNECTED;
				// agent-side registered subscribers are already cleared when the socket dies.
			}
		}
	}
}


// reimplemented (icomp::CComponentBase)

void CSubscriptionControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		RegisterModel(m_modelCompPtr.GetPtr(), MI_AGENT_COLLECTION);
	}

	if (m_agentStatusModelCompPtr.IsValid()){
		RegisterModel(m_agentStatusModelCompPtr.GetPtr(), MI_AGENT_STATUS_COLLECTION);
	}

	// Agents already present in the mirror (DB load / already connected).
	RefreshSubscriptionsFromAgentCollection(false);
}


void CSubscriptionControllerComp::OnComponentDestroyed()
{
	UnregisterAllModels();

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql
