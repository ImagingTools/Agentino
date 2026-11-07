// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/IConnectionTokenStore.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentConnectionTokens_fwd.h>


namespace agentgql
{


/**
	The tokens of the output connections of the services of this agent, as the Agentino server hands them over.
	The values of the tokens never leave the agent.
*/
class CAgentConnectionTokenControllerComp: public sdl::V1_0::agentino::CAgentConnectionTokensGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CAgentConnectionTokensGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentConnectionTokenControllerComp);
		I_ASSIGN(m_tokenStoreCompPtr, "TokenStore", "Tokens of the output connections", true, "TokenStore");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CAgentConnectionTokensGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CConnectionTokenState OnGetAgentConnectionToken(
				const sdl::V1_0::agentino::CGetAgentConnectionTokenGqlRequest& getAgentConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CConnectionTokenChangePayload OnStoreAgentConnectionToken(
				const sdl::V1_0::agentino::CStoreAgentConnectionTokenGqlRequest& storeAgentConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CConnectionTokenChangePayload OnRemoveAgentConnectionToken(
				const sdl::V1_0::agentino::CRemoveAgentConnectionTokenGqlRequest& removeAgentConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	I_REF(agentinodata::IConnectionTokenStore, m_tokenStoreCompPtr);
};


} // namespace agentgql


