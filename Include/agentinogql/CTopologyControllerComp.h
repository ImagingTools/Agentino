// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once



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
	Server-side topology controller.

	Builds topology from in-process agent/service collections.
	SaveTopology persists layout coordinates centrally.
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
	// Build topology from in-process collections
	sdl::V1_0::agentino::CTopology GetLegacyTopology() const;

	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
};


} // namespace agentinogql
