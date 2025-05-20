#include <agentinogql/CAgentsSubscriberProxyControllerComp.h>


// Agentino includes
#include <agentinodata/IServiceController.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::IGqlSubscriberController)

bool CAgentsSubscriberProxyControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool retVal = BaseClass::IsRequestSupported(gqlRequest);
	QByteArray agentId = gqlRequest.GetHeader("clientid");

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


bool CAgentsSubscriberProxyControllerComp::UnregisterSubscription(const QByteArray& subscriptionId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return false;
	}

	bool retVal = BaseClass::UnregisterSubscription(subscriptionId);
	if (retVal){
		for (auto it = m_remoteSubscriptions.constBegin(); it != m_remoteSubscriptions.constEnd(); ++it) {
			if (it.value() == subscriptionId) {
				m_subscriptionManagerCompPtr->UnregisterSubscription(it.key());
				m_remoteSubscriptions.remove(it.key());
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
		
		QJsonObject jsonData = document.object().value(subscriptionTypeId).toObject();
		QJsonDocument documentBody;
		documentBody.setObject(jsonData);
		QByteArray body = documentBody.toJson(QJsonDocument::Compact);
		
		for (RequestNetworks& requestNetworks: m_registeredSubscribers){
			if (requestNetworks.networkRequests.contains(registerSubscriptionId)){
				PublishData(subscriptionTypeId, body);

				break;
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


