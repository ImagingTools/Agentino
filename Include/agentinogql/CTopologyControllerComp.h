// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>

// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology_fwd.h>


namespace agentinogql
{


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

private:
	struct SnapshotInfo
	{
		sdl::V1_0::agentino::CTopology snapshot;
		QDateTime lastSnapshotAt;
		bool isStale = true;
	};

	sdl::V1_0::agentino::CTopology GetLegacyTopology() const;
	sdl::V1_0::agentino::CTopology GetAggregatedTopology(const ::imtgql::CGqlRequest& gqlRequest) const;
	sdl::V1_0::agentino::CTopology GetAgentSnapshot(
				const ::imtgql::CGqlRequest& gqlRequest,
				const QByteArray& agentId,
				bool& isFresh,
				QString& errorMessage) const;
	void ApplyLayoutCoordinates(sdl::V1_0::agentino::CTopology& topology) const;
	void MarkServicesAsStale(sdl::V1_0::agentino::CTopology& topology) const;
	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	mutable QMap<QByteArray, SnapshotInfo> m_snapshotInfoMap;
};


} // namespace agentinodata
