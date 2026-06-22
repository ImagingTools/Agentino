// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// ACF includes
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceCompositeInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology_fwd.h>


namespace agentgql
{


/**
	Agent local topology controller.
	Provides the topology of the agent's own services based on the local service collection,
	so that the agent remains the source of truth for its own service topology
	even if the central Agentino server is not available.
*/
class CAgentTopologyControllerComp: public sdl::V1_0::agentino::CTopologyGqlHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentTopologyControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Local service collection of the agent", true, "ServiceCollection");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Local topology layout collection", true, "TopologyCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Local service composite info", true, "ServiceCompositeInfo");
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Parameter providing the agent's client-ID", false, "ClientIdParam");
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
	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
};


} // namespace agentgql
