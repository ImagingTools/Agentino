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


bool CAgentsSubscriberProxyControllerComp::RegisterSubscription(
			const QByteArray& subscriptionId,
			const imtgql::CGqlRequest& gqlRequest,
			const imtrest::IRequest& networkRequest,
			QString& errorMessage)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		errorMessage = "Internal error";

		return false;
	}

	bool retVal = BaseClass::RegisterSubscription(subscriptionId, gqlRequest, networkRequest, errorMessage);
	if (retVal){
		QByteArray remoteSubscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(gqlRequest, this);
		m_remoteSubscriptions.insert(remoteSubscriptionId, subscriptionId);
	}

	return retVal;
}


bool CAgentsSubscriberProxyControllerComp::UnRegisterSubscription(const QByteArray& subscriptionId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){

		return false;
	}

	bool retVal = BaseClass::UnRegisterSubscription(subscriptionId);
	if (retVal){
		for (QByteArray remoteSubscriptionId: m_remoteSubscriptions){
			if (m_remoteSubscriptions.value(remoteSubscriptionId) == subscriptionId){
				m_subscriptionManagerCompPtr->UnregisterSubscription(remoteSubscriptionId);
				m_remoteSubscriptions.remove(remoteSubscriptionId);
				retVal = true;

				break;
			}
		}
	}

	return retVal;
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
		QByteArray registerSubscriptionId = m_remoteSubscriptions.value(subscriptionId);
		if (registerSubscriptionId.isEmpty()){
			return;
		}
		for (RequestNetworks& requestNetworks: m_registeredSubscribers){
			if (requestNetworks.networkRequests.contains(registerSubscriptionId)){
				QJsonObject jsonData = document.object().value(subscriptionTypeId).toObject();

				QJsonDocument documentBody;
				documentBody.setObject(jsonData);

				QByteArray body = documentBody.toJson(QJsonDocument::Compact);
				SetAllSubscriptions(subscriptionTypeId, body);
			}
		}
	}
}


void CAgentsSubscriberProxyControllerComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


} // namespace agentinogql


