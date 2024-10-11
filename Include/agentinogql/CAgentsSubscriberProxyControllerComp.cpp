#include <agentinogql/CAgentsSubscriberProxyControllerComp.h>


// ImtCore includes
#include <imtbase/CTreeItemModel.h>
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceController.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::IGqlSubscriberController)

bool CAgentsSubscriberProxyControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool retVal = BaseClass::IsRequestSupported(gqlRequest);
	QByteArray agentId = gqlRequest.GetHeader("clientId");

	return retVal && !agentId.isEmpty();
}


// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CAgentsSubscriberProxyControllerComp::OnResponseReceived(const QByteArray & subscriptionId, const QByteArray & subscriptionData)
{
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);

	QStringList keys = document.object().keys();
	if (!keys.isEmpty()){
		QByteArray subscriptionTypeId = keys[0].toUtf8();
		QList<QByteArray> subscriptionIds = m_registeredAgents.values();
		if (subscriptionIds.contains(subscriptionId)){
			QJsonObject jsonData = document.object().value(subscriptionTypeId).toObject();

			QJsonDocument documentBody;
			documentBody.setObject(jsonData);

			QByteArray body = documentBody.toJson(QJsonDocument::Compact);
			SetAllSubscriptions(subscriptionTypeId, body);
		}
	}
}


void CAgentsSubscriberProxyControllerComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentsSubscriberProxyControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& /*changeSet*/)
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
				for (int index = 0; index < m_commandIdsAttrPtr.GetCount(); index++){
					imtgql::CGqlRequest gqlAddRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION, m_commandIdsAttrPtr[index]);
					imtgql::CGqlObject subscriptionInput;
					// imtgql::CGqlObject subscriptionAddition;
					// subscriptionAddition.InsertField("clientId", QString(agentId));
					// subscriptionInput.InsertField("addition", subscriptionAddition);
					gqlAddRequest.AddParam("input", subscriptionInput);

					imtgql::CGqlObject subscriptionField;
					subscriptionField.InsertField("id");
					subscriptionField.InsertField("status");
					gqlAddRequest.AddField("data", subscriptionField);

					imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
					imtgql::IGqlContext::Headers headers;
					headers.insert("clientId",agentId);
					gqlContextPtr->SetHeaders(headers);
					gqlAddRequest.SetGqlContext(gqlContextPtr);

					QByteArray subscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlAddRequest, this);
					m_registeredAgents.insert(agentId, subscriptionId);
				}
			}
		}
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentsSubscriberProxyControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->AttachObserver(this);
	}
}


void CAgentsSubscriberProxyControllerComp::OnComponentDestroyed()
{
	if (m_modelCompPtr.IsValid() && m_modelCompPtr->IsAttached(this)){
		m_modelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql


