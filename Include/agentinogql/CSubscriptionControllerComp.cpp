// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CSubscriptionControllerComp.h"


// Qt includes
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

// ImtCore includes
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CSubscriptionControllerComp::OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData)
{
	// A service was created / changed / removed directly on an agent: reconcile the server mirror.
	if (m_registeredServiceCollectionAgents.values().contains(subscriptionId)){
		HandleServiceCollectionChanged(subscriptionId, subscriptionData);

		return;
	}

	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	QJsonObject subscriptionObject = document.object().value("OnAgentServiceStatusChanged").toObject();
	QList<QByteArray> subscriptionIds = m_registeredAgents.values();
	if (subscriptionIds.contains(subscriptionId)){
		notifierInfo.serviceId = subscriptionObject.value("serviceid").toString().toUtf8();

		QString status = subscriptionObject.value(agentino::ServiceStatus::s_Key).toString();

		agentinodata::IServiceStatusInfo::FromString(status.toUtf8(), notifierInfo.serviceStatus);
		changeSet.SetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierInfo));
		istd::CChangeNotifier notifier(this, &changeSet);
	}
}


void CSubscriptionControllerComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


// private methods

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

	imtgql::CGqlFieldObject subscriptionField;
	subscriptionField.InsertField("id");
	subscriptionField.InsertField("status");
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
		qDebug() << "CSubscriptionControllerComp: live collection subscription registered for agent" << agentId << collectionSubscriptionId;
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


void CSubscriptionControllerComp::EnsureLiveSubscriptionsForAgent(const QByteArray& agentId, bool forceReregisterCollection)
{
	if (agentId.isEmpty()){
		return;
	}

	if (!m_registeredAgents.contains(agentId)){
		RegisterStatusSubscription(agentId);
	}

	if (!m_serviceSynchronizerCompPtr.IsValid()){
		return;
	}

	if (forceReregisterCollection && m_registeredServiceCollectionAgents.contains(agentId)){
		// Drop stale WS registration on the agent (e.g. after reconnect) and open a fresh one.
		UnregisterCollectionSubscription(agentId);
	}

	if (!m_registeredServiceCollectionAgents.contains(agentId)){
		RegisterCollectionSubscription(agentId);
	}
}


void CSubscriptionControllerComp::RefreshSubscriptionsFromAgentCollection()
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
		// AgentAdd / reconnect updates LastConnection (CF_OBJECT_DATA_CHANGED) while the agent
		// is online — force re-register collection so live push works without waiting for a full process restart.
		EnsureLiveSubscriptionsForAgent(agentId, true);
	}
}


// reimplemented (imod::CMultiModelDispatcherBase)

void CSubscriptionControllerComp::OnModelChanged(int modelId, const istd::IChangeable::ChangeSet& changeSet)
{
	if (modelId == MI_AGENT_COLLECTION){
		// Ignore CF_SERVICE_* mirror churn (same AgentCollection object) — only membership / AgentAdd.
		const bool agentMembershipChange =
					changeSet.Contains(istd::IChangeable::CF_ANY) ||
					changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED) ||
					changeSet.Contains(imtbase::ICollectionInfo::CF_REMOVED) ||
					changeSet.Contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED) ||
					changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_RENAMED);

		if (agentMembershipChange){
			RefreshSubscriptionsFromAgentCollection();
		}

		return;
	}

	if (modelId == MI_AGENT_STATUS_COLLECTION){
		// Agent went online/offline — re-open live collection subscription while the WS is up.
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
	RefreshSubscriptionsFromAgentCollection();
}


void CSubscriptionControllerComp::OnComponentDestroyed()
{
	UnregisterAllModels();

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql

