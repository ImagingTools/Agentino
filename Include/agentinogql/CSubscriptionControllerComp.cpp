#include "CSubscriptionControllerComp.h"


// ImtCore includes
#include <imtbase/CTreeItemModel.h>
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CSubscriptionControllerComp::OnResponseReceived(const QByteArray & subscriptionId, const QByteArray & subscriptionData)
{
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	QJsonObject subscriptionObject = document.object().value("OnServiceStateChanged").toObject();
	QList<QByteArray> subscriptionIds = m_registeredAgents.values();
	if (subscriptionIds.contains(subscriptionId)){
		notifierInfo.serviceId = subscriptionObject.value("serviceId").toString().toUtf8();

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


// reimplemented (imod::CSingleModelObserverBase)

void CSubscriptionControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& /*changeSet*/)
{
	if (m_subscriptionManagerCompPtr.IsValid() && m_agentCollectionCompPtr.IsValid()){
		imtbase::ICollectionInfo::Ids agentCollectionIds = m_agentCollectionCompPtr->GetElementIds();

		for (const QByteArray& registeredAgentId: m_registeredAgents){
			if (!agentCollectionIds.contains(registeredAgentId)){
				m_subscriptionManagerCompPtr->UnregisterSubscription(m_registeredAgents[registeredAgentId]);
				m_registeredAgents.remove(registeredAgentId);
			}
		}

		for (const QByteArray& agentId: agentCollectionIds){
			if(!m_registeredAgents.contains(agentId)){
				imtgql::CGqlRequest gqlAddRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, "OnServiceStateChanged");
				imtgql::CGqlObject subscriptionInput;
				imtgql::CGqlObject subscriptionAddition;
				subscriptionAddition.InsertField("clientId", QString(agentId));
				subscriptionInput.InsertField("addition", subscriptionAddition);
				gqlAddRequest.AddParam("input", subscriptionInput);

				imtgql::CGqlObject subscriptionField;
				subscriptionField.InsertField("id");
				subscriptionField.InsertField("status");
				gqlAddRequest.AddField("data", subscriptionField);
				QByteArray subscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlAddRequest, this);
				m_registeredAgents.insert(agentId, subscriptionId);
			}
		}
	}
}


// reimplemented (icomp::CComponentBase)

void CSubscriptionControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->AttachObserver(this);
	}
}


void CSubscriptionControllerComp::OnComponentDestroyed()
{
	if (m_modelCompPtr.IsValid() && m_modelCompPtr->IsAttached(this)){
		m_modelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql


