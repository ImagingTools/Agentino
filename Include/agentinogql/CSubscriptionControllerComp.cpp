#include "CSubscriptionControllerComp.h"


// ImtCore includes
#include <imtbase/CTreeItemModel.h>
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceController.h>


namespace agentinogql
{


// protected methods

void CSubscriptionControllerComp::OnComponentCreated()
{
//	if (m_subscriptionManagerCompPtr.IsValid()){
//		imtgql::CGqlRequest gqlAddRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, "OnServiceStateChanged");
//		imtgql::CGqlObject subscriptionField("data");
//		subscriptionField.InsertField("id");
//		subscriptionField.InsertField("status");
//		gqlAddRequest.AddField(subscriptionField);
//		m_serviceStatusSubsriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlAddRequest, this);
//	}
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()) {
		m_modelCompPtr->AttachObserver(this);
	}
}


void CSubscriptionControllerComp::OnResponseReceived(const QByteArray & subscriptionId, const QByteArray & subscriptionData)
{
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	QJsonObject subscriptionObject = document.object().value("OnServiceStateChanged").toObject();
	QList<QByteArray> subscriptionIds = m_registeredAgents.values();
	if (subscriptionIds.contains(subscriptionId)){
		notifierInfo.serviceId = subscriptionObject.value("serviceId").toString().toUtf8();
		QString status = subscriptionObject.value("serviceStatus").toString();
		if (status == "Starting"){
			notifierInfo.serviceStatus = QProcess::Starting;
		}
		if (status == "Running"){
			notifierInfo.serviceStatus = QProcess::Running;
		}
		if (status == "NotRunning"){
			notifierInfo.serviceStatus = QProcess::NotRunning;
		}
		changeSet.SetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierInfo));
		istd::CChangeNotifier notifier(this, &changeSet);
	}

//	if (m_gqlCollectionCompPtr.IsValid()){
//		istd::CChangeNotifier notifier(m_gqlCollectionCompPtr.GetPtr(), &changeSet);
//	}
}


void CSubscriptionControllerComp::OnSubscriptionStatusChanged(const QByteArray & subscriptionId, const SubscriptionStatus & status, const QString & message)
{
}


// reimplemented (imod::CSingleModelObserverBase)

void CSubscriptionControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (m_subscriptionManagerCompPtr.IsValid() && m_agentCollectionCompPtr.IsValid()){
		imtbase::ICollectionInfo::Ids agentCollectionIds = m_agentCollectionCompPtr->GetElementIds();
//		imtbase::ICollectionInfo::Ids ids = m_agentCollectionCompPtr->GetElementIds();
//		QList<QByteArray> agentCollectionIds;
//		for (QByteArray collectionId: ids){
//			agentinodata::CIdentifiableAgentInfo* agentPtr = nullptr;
//			imtbase::IObjectCollection::DataPtr agentDataPtr;
//			if (m_agentCollectionCompPtr->GetObjectData(collectionId, agentDataPtr)){
//				agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentDataPtr.GetPtr());
//			}

//			if (agentPtr != nullptr){
//				QByteArray agentId = agentPtr->GetObjectUuid();
//				agentCollectionIds.append(agentId);
//			}
//		}

		for (QByteArray registeredAgentId: m_registeredAgents){
			if (!agentCollectionIds.contains(registeredAgentId)){
				m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredAgents[registeredAgentId]);
				m_registeredAgents.remove(registeredAgentId);
			}
		}

		for (QByteArray agentId: agentCollectionIds){
			if(!m_registeredAgents.contains(agentId)){
				imtgql::CGqlRequest gqlAddRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, "OnServiceStateChanged");
				imtgql::CGqlObject subscriptionInput("input");
				imtgql::CGqlObject subscriptionAddition("addition");
				subscriptionAddition.InsertField("clientId", QString(agentId));
				subscriptionInput.InsertField("addition", subscriptionAddition);
				gqlAddRequest.AddParam(subscriptionInput);

				imtgql::CGqlObject subscriptionField("data");
				subscriptionField.InsertField("id");
				subscriptionField.InsertField("status");
				gqlAddRequest.AddField(subscriptionField);
				QByteArray subscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlAddRequest, this);
				m_registeredAgents.insert(agentId, subscriptionId);
			}
		}
	}
}


} // namespace agentinogql


