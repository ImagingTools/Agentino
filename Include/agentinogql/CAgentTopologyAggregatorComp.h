// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QMutex>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <icomp/CComponentBase.h>
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/IGqlClient.h>
#include <imtcom/IConnectionStatusProvider.h>

// Agentino includes
#include <agentinodata/IServiceCompositeInfo.h>
#include <agentinodata/IAgentStatusInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentTopology_fwd.h>


namespace agentinogql
{


/**
	Aggregates topology information from all connected agents.
	Implements a pull-based model where the server queries each agent's
	local topology endpoint and caches the results.
	When an agent is offline, the last known topology is retained
	with an appropriate connection status.
*/
class CAgentTopologyAggregatorComp:
			public ilog::CLoggerComponentBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;
	typedef imod::TSingleModelObserverBase<istd::IChangeable> BaseClass2;

	I_BEGIN_COMPONENT(CAgentTopologyAggregatorComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection", false, "AgentStatusCollection");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", false, "ServiceCompositeInfo");
		I_ASSIGN(m_loginStatusProviderCompPtr, "LoginStatusProvider", "Login status provider", false, "LoginStatusProvider");
		I_ASSIGN_TO(m_loginStatusModelCompPtr, m_loginStatusProviderCompPtr, false);
	I_END_COMPONENT;

	/**
		Cached topology data for a single agent.
	*/
	struct AgentTopologyCache
	{
		QByteArray agentId;
		QString agentName;
		sdl::V1_0::agentino::AgentConnectionStatus connectionStatus;
		QDateTime lastSeen;
		QList<sdl::V1_0::agentino::CLocalServiceInfo> services;
	};

	/**
		Get cached topology for all agents.
	*/
	QList<AgentTopologyCache> GetAggregatedTopology() const;

	/**
		Update topology cache from an agent's local topology response.
		This is called when the server receives topology data from an agent,
		either via pull or push.
	*/
	void UpdateAgentTopology(const QByteArray& agentId, const sdl::V1_0::agentino::CLocalTopology& localTopology);

	/**
		Mark an agent as offline in the cache, preserving last known topology.
	*/
	void SetAgentOffline(const QByteArray& agentId);

	/**
		Mark an agent as online in the cache.
	*/
	void SetAgentOnline(const QByteArray& agentId);

	/**
		Remove an agent from the topology cache.
	*/
	void RemoveAgent(const QByteArray& agentId);

protected:
	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusProviderCompPtr);
	I_REF(imod::IModel, m_loginStatusModelCompPtr);

private:
	mutable QMutex m_cacheMutex;
	QMap<QByteArray, AgentTopologyCache> m_topologyCache;
};


} // namespace agentinogql
