// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/ILocalTopologyInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology_fwd.h>


namespace agentgql
{


/**
	Agent-side GQL handler for local topology operations.
	Implements GetLocalTopology (query) and SaveLocalTopology (mutation)
	so each agent can serve and persist its own service layout independently
	of the central Agentino server.
*/
class CLocalTopologyControllerComp: public sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CLocalTopologyControllerComp);
		I_ASSIGN(m_localTopologyInfoCompPtr, "LocalTopologyInfo", "Local topology storage", true, "LocalTopologyInfo");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CLocalTopology OnGetLocalTopology(
				const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& getLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSaveLocalTopologyResponse OnSaveLocalTopology(
				const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& saveLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	I_REF(agentinodata::ILocalTopologyInfo, m_localTopologyInfoCompPtr);
};


} // namespace agentgql
