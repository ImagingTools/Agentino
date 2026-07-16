// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>
#include <imod/CMultiModelDispatcherBase.h>

// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinogql/IServiceCollectionSynchronizer.h>


namespace agentinogql
{


/**
	Server-side subscription hub for connected agents.

	Registers (and re-registers on agent reconnect) live WebSocket subscriptions:
	- OnAgentServiceStatusChanged
	- OnAgentServicesCollectionChanged  → IServiceCollectionSynchronizer (live mirror sync)
*/
class CSubscriptionControllerComp:
			public ilog::CLoggerComponentBase,
			public imod::CMultiModelDispatcherBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;
	typedef imod::CMultiModelDispatcherBase BaseClass2;

	enum ModelId
	{
		MI_AGENT_COLLECTION = 0,
		MI_AGENT_STATUS_COLLECTION
	};

	I_BEGIN_COMPONENT(CSubscriptionControllerComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", true, "SubscriptionManager");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_modelCompPtr, "Model", "Agent collection model (membership / AgentAdd)", true, "Model");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent online/offline status (re-register live WS subs on connect)", false, "AgentStatusCollection");
		I_ASSIGN_TO(m_agentStatusModelCompPtr, m_agentStatusCollectionCompPtr, false);
		I_ASSIGN(m_serviceSynchronizerCompPtr, "ServiceSynchronizer", "Refreshes the mirror on agent service-collection changes", false, "ServiceSynchronizer");
	I_END_COMPONENT;

protected:
	// reimplemented (imtclientgql::IGqlSubscriptionClient)
	virtual void OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData) override;
	virtual void OnSubscriptionStatusChanged(const QByteArray& subscriptionId, const SubscriptionStatus& status, const QString& message) override;

	// reimplemented (imod::CMultiModelDispatcherBase)
	virtual void OnModelChanged(int modelId, const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

private:
	// Reconciles the server-side mirror when an agent reports a service-collection change.
	void HandleServiceCollectionChanged(const QByteArray& subscriptionId, const QByteArray& subscriptionData);

	// Drop subs for removed agents; ensure status + collection subs for every agent in the collection.
	void RefreshSubscriptionsFromAgentCollection();

	// Force re-register live collection (and status if missing) for one agent - used on CONNECTED.
	void EnsureLiveSubscriptionsForAgent(const QByteArray& agentId, bool forceReregisterCollection);

	void RegisterStatusSubscription(const QByteArray& agentId);
	void RegisterCollectionSubscription(const QByteArray& agentId);
	void UnregisterStatusSubscription(const QByteArray& agentId);
	void UnregisterCollectionSubscription(const QByteArray& agentId);

	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
	I_REF(imod::IModel, m_agentStatusModelCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(IServiceCollectionSynchronizer, m_serviceSynchronizerCompPtr);

	QMap<QByteArray, QByteArray> m_registeredAgents; // <agentId, subscriptionId> for OnAgentServiceStatusChanged
	QMap<QByteArray, QByteArray> m_registeredServiceCollectionAgents; // <agentId, subscriptionId> for OnAgentServicesCollectionChanged
};


} // namespace agentinogql

