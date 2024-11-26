#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlSubscriberControllerCompBase.h>


namespace agentinogql
{


class CAgentsSubscriberProxyControllerComp:
			public imtservergql::CGqlSubscriberControllerCompBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef imtservergql::CGqlSubscriberControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentsSubscriberProxyControllerComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", true, "SubscriptionManager");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlSubscriberController)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
	virtual bool RegisterSubscription(
				const QByteArray& subscriptionId,
				const imtgql::CGqlRequest& gqlRequest,
				const imtrest::IRequest& networkRequest,
				QString& errorMessage) override;
	virtual bool UnRegisterSubscription(const QByteArray& subscriptionId) override;

	// reimplemented (imtclientgql::IGqlSubscriptionClient)
	virtual void OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData) override;
	virtual void OnSubscriptionStatusChanged(const QByteArray& subscriptionId, const SubscriptionStatus& status, const QString& message) override;

private:
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);

	QMap<QByteArray, QByteArray> m_registeredAgents; // <agentId, subscriptionId>
	QMap<QByteArray, QByteArray> m_remoteSubscriptions; // <remoteSubscriptionId, subscriptionId>
};


} // namespace agentinogql


