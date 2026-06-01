// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMutex>
#include <QtCore/QDateTime>

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology_fwd.h>


namespace agentinogql
{


/**
	Server-side topology controller with pull-based agent aggregation.

	Supports two modes controlled by the AGENTINO_TOPOLOGY_SNAPSHOT_AGGREGATOR
	environment variable:
	- Legacy mode (default): builds topology from in-process agent/service collections
	- Aggregation mode: pulls topology snapshots from each agent via GQL,
	  caches them, and applies centrally-stored layout coordinates

	SaveTopology always persists layout coordinates centrally (both modes).
*/
class CTopologyControllerComp:
			public imtclientgql::TClientRequestManagerCompWrap<
								sdl::V1_0::agentino::CTopologyGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
		sdl::V1_0::agentino::CTopologyGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CTopologyControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Topology collection", true, "TopologyCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
	I_END_COMPONENT;

	// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CTopology OnGetTopology(
				const sdl::V1_0::agentino::CGetTopologyGqlRequest& getTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSaveTopologyResponse OnSaveTopology(
				const sdl::V1_0::agentino::CSaveTopologyGqlRequest& saveTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

private:
	// Legacy topology (direct in-process collection query)
	sdl::V1_0::agentino::CTopology GetLegacyTopology() const;

	// Pull-based aggregated topology from agents
	sdl::V1_0::agentino::CTopology GetAggregatedTopology(const ::imtgql::CGqlRequest& gqlRequest) const;

	// Pull a single agent's topology snapshot
	bool GetAgentSnapshot(
				const QByteArray& agentId,
				const ::imtgql::CGqlRequest& gqlRequest,
				sdl::V1_0::agentino::CTopology& snapshot) const;

	// Apply stored layout coordinates to all services
	void ApplyLayoutCoordinates(sdl::V1_0::agentino::CTopology& topology) const;

	// Mark all services in a topology as stale (agent unreachable)
	void MarkServicesAsStale(sdl::V1_0::agentino::CTopology& topology) const;

	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

	struct SnapshotInfo
	{
		sdl::V1_0::agentino::CTopology snapshot;
		QDateTime lastSnapshotAt;
	};

	bool m_useAggregation = false;
	mutable QMutex m_snapshotMutex;
	mutable QMap<QByteArray, SnapshotInfo> m_snapshotCache;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
};


} // namespace agentinogql
