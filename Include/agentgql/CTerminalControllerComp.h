// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/ITerminalController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal_fwd.h>


namespace agentgql
{


/**
	Agent side GraphQL resolver for the remote terminal feature.

	Thin per-request adapter: runs inside the AgentinoAgent's GQL handler tree (recreated
	per request) and delegates every call to the persistent
	\ref agentinodata::CTerminalSessionManagerComp sibling, which actually owns the shell
	processes and their buffered output across requests.

	\ingroup Terminal
*/
class CTerminalControllerComp: public sdl::V1_0::agentino::CTerminalGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CTerminalGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerComp);
		I_ASSIGN(m_terminalSessionManagerCompPtr, "TerminalSessionManager", "Terminal session manager used to run shell sessions", true, "TerminalSessionManager");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CTerminalGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CShellTypeListPayload OnListShellTypes(
				const sdl::V1_0::agentino::CListShellTypesGqlRequest& listShellTypesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTerminalOutputResponse OnGetTerminalOutput(
				const sdl::V1_0::agentino::CGetTerminalOutputGqlRequest& getTerminalOutputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::COpenTerminalSessionResponse OnOpenTerminalSession(
				const sdl::V1_0::agentino::COpenTerminalSessionGqlRequest& openTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSendTerminalInputResponse OnSendTerminalInput(
				const sdl::V1_0::agentino::CSendTerminalInputGqlRequest& sendTerminalInputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CInterruptTerminalSessionResponse OnInterruptTerminalSession(
				const sdl::V1_0::agentino::CInterruptTerminalSessionGqlRequest& interruptTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CCloseTerminalSessionResponse OnCloseTerminalSession(
				const sdl::V1_0::agentino::CCloseTerminalSessionGqlRequest& closeTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	I_REF(agentinodata::ITerminalController, m_terminalSessionManagerCompPtr);
};


} // namespace agentgql
