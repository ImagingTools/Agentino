#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtbase/IObjectCollection.h>
#include <imtgql/CGqlSubscriberControllerCompBase.h>


namespace agentinogql
{


class CAgentsSubscriberProxyControllerComp:
			public imtgql::CGqlSubscriberControllerCompBase,
			public imod::CSingleModelObserverBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef imtgql::CGqlSubscriberControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentsSubscriberProxyControllerComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", true, "SubscriptionManager");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_modelCompPtr, "Model", "Model", true, "Model");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlSubscriberController)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;

	// reimplemented (imtclientgql::IGqlSubscriptionClient)
	virtual void OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData) override;
	virtual void OnSubscriptionStatusChanged(const QByteArray& subscriptionId, const SubscriptionStatus& status, const QString& message) override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

private:
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);

	QMap<QByteArray, QByteArray> m_registeredAgents; // <agentId, subscriptionId>
};


} // namespace agentinogql


