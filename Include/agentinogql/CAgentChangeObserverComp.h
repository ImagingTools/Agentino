// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QString>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/CMultiModelDispatcherBase.h>

// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtbase/IObjectCollection.h>
#include <imtcom/IConnectionStatusProvider.h>

// Agentino includes
#include <agentinodata/IServiceStatusInfo.h>
#include <agentinodata/IAgentStatusInfo.h>
#include <agentinodata/IServiceManager.h>
#include <agentinogql/IEnrollmentController.h>
#include <agentinogql/IServiceCollectionSynchronizer.h>


namespace agentinogql
{


/**
	The single server-side observer of agent changes.

	One component watches everything an agent reports and applies the server's reaction to it —
	no other component tracks agents:
	- agent WS connect / disconnect (login status)  → agent status, per-service status reset,
	  connection-endpoint availability, and (re)registration of live subscriptions;
	- OnAgentServiceStatusChanged                    → ServiceStatusCollection;
	- OnAgentServicesCollectionChanged               → mirror sync via IServiceCollectionSynchronizer.

	Status path (both steps are required):
	1. ApplyServiceStatus writes ServiceStatusCollection (CollectionSubscriber / topology).
	2. CChangeNotifier(CN_STATUS_CHANGED) wakes ServiceStatusSubscriberController which
	   publishes OnServiceStatusChanged to the GUI WebSocket clients.

	OnResponseReceived is invoked from the WebSocket I/O thread (RequestClientHandler path)
	while StartService/StopService run on GQL worker threads with nested event loops. All
	mirror / collection mutation is therefore always queued onto this QObject's thread
	(main) to avoid races that freeze workers and leave the server unable to handle
	subsequent commands (e.g. SaveTopology) until the client reconnects.
*/
class CAgentChangeObserverComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			public imod::CMultiModelDispatcherBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;
	typedef imod::CMultiModelDispatcherBase BaseClass2;

	enum ModelId
	{
		MI_AGENT_COLLECTION = 0,
		MI_AGENT_STATUS_COLLECTION,
		MI_LOGIN_STATUS
	};

	I_BEGIN_COMPONENT(CAgentChangeObserverComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", true, "SubscriptionManager");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_modelCompPtr, "Model", "Agent collection model (membership / AgentAdd)", true, "Model");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent online/offline status (re-register live WS subs on connect)", false, "AgentStatusCollection");
		I_ASSIGN_TO(m_agentStatusModelCompPtr, m_agentStatusCollectionCompPtr, false);
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection updated from OnAgentServiceStatusChanged", false, "ServiceStatusCollection");
		I_ASSIGN(m_serviceSynchronizerCompPtr, "ServiceSynchronizer", "Refreshes the mirror on agent service-collection changes", false, "ServiceSynchronizer");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Live-path gate: only Approved agents ingest", false, "EnrollmentStore");
		// Absorbed from CAgentConnectionObserverComp: the same component now also reacts to
		// agent WS connect/disconnect (single agent-change observer).
		I_ASSIGN(m_loginStatusProviderCompPtr, "LoginStatusProvider", "Agent WS connection status (connect/disconnect)", false, "LoginStatusProvider");
		I_ASSIGN_TO(m_loginStatusModelCompPtr, m_loginStatusProviderCompPtr, false);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections (reset statuses on disconnect)", false, "ServiceManager");
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
	// Dispatches payload handling to this object's thread when called from the WS I/O thread.
	void QueueHandleResponse(const QByteArray& subscriptionId, const QByteArray& subscriptionData);
	void HandleResponseOnOwnerThread(const QByteArray& subscriptionId, const QByteArray& subscriptionData);

	// Applies agent status push into ServiceStatusCollection and notifies model observers
	// (ServiceStatusSubscriberController → OnServiceStatusChanged).
	void HandleServiceStatusChanged(const QByteArray& subscriptionId, const QByteArray& subscriptionData);

	// Reconciles the server-side mirror when an agent reports a service-collection change.
	void HandleServiceCollectionChanged(const QByteArray& subscriptionId, const QByteArray& subscriptionData);

	// Drop subs for removed agents; ensure status + collection subs for every agent in the collection.
	// forceReregisterExisting=false only fills missing entries (safe for membership noise).
	void RefreshSubscriptionsFromAgentCollection(bool forceReregisterExisting);

	// forceReregister: drop and re-open both live subscriptions (status + collection) for one agent.
	// Used only on AS_CONNECTED, where agent-side subscriber lists were wiped on disconnect.
	void EnsureLiveSubscriptionsForAgent(const QByteArray& agentId, bool forceReregister);

	void RegisterStatusSubscription(const QByteArray& agentId);
	void RegisterCollectionSubscription(const QByteArray& agentId);
	void UnregisterStatusSubscription(const QByteArray& agentId);
	void UnregisterCollectionSubscription(const QByteArray& agentId);

	// Result of writing an agent status push into ServiceStatusCollection.
	enum class ApplyStatusResult
	{
		Failed,     // collection missing / cast failed / write failed
		Unchanged,  // entry already had this status (no collection notify)
		Changed     // SetObjectData / InsertNewObject succeeded (CollectionSubscriber fires)
	};

	ApplyStatusResult ApplyServiceStatus(
				const QByteArray& serviceId,
				agentinodata::IServiceStatusInfo::ServiceStatus status);

	/** True when no gate is wired, or enrollment record is Approved. */
	bool IsAgentIngestionAllowed(const QByteArray& agentId) const;
	void DropLiveSubscriptionsForAgent(const QByteArray& agentId);

	// Absorbed from CAgentConnectionObserverComp: react to agent WS connect / disconnect.
	void HandleAgentConnectionChanged(const istd::IChangeable::ChangeSet& changeSet);
	void SetAgentStatus(const QByteArray& agentId, agentinodata::IAgentStatusInfo::AgentStatus status);
	void ResetAgentServiceStatuses(const QByteArray& agentId);

	static bool ParseServiceStatus(
				const QString& statusText,
				agentinodata::IServiceStatusInfo::ServiceStatus& status);
	static QJsonObject ExtractStatusPayload(const QJsonObject& root);

	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
	I_REF(imod::IModel, m_agentStatusModelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(IServiceCollectionSynchronizer, m_serviceSynchronizerCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusProviderCompPtr);
	I_REF(imod::IModel, m_loginStatusModelCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);

	QMap<QByteArray, QByteArray> m_registeredAgents; // <agentId, subscriptionId> for OnAgentServiceStatusChanged
	QMap<QByteArray, QByteArray> m_registeredServiceCollectionAgents; // <agentId, subscriptionId> for OnAgentServicesCollectionChanged
};


} // namespace agentinogql
