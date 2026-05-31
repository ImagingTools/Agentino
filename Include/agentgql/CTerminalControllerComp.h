// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/ITerminalController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal.h>


namespace agentgql
{


/**
	Agent side GraphQL resolver for the remote terminal feature.

	Runs inside the AgentinoAgent process and delegates every request to the local
	\ref agentinodata::ITerminalController, which owns the shell processes.

	\ingroup Terminal
*/
class CTerminalControllerComp: public sdl::agentino::Terminal::CGraphQlHandlerCompBase
{
public:
	typedef CGraphQlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerComp);
		I_ASSIGN(m_terminalControllerCompPtr, "TerminalController", "Terminal controller used to run shell sessions", true, "TerminalController");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::agentino::Terminal::CGraphQlHandlerCompBase)
	virtual sdl::agentino::Terminal::CShellTypeListPayload OnListShellTypes(
				const sdl::agentino::Terminal::CListShellTypesGqlRequest& listShellTypesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Terminal::CTerminalOutputResponse OnGetTerminalOutput(
				const sdl::agentino::Terminal::CGetTerminalOutputGqlRequest& getTerminalOutputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Terminal::COpenTerminalSessionResponse OnOpenTerminalSession(
				const sdl::agentino::Terminal::COpenTerminalSessionGqlRequest& openTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Terminal::CSendTerminalInputResponse OnSendTerminalInput(
				const sdl::agentino::Terminal::CSendTerminalInputGqlRequest& sendTerminalInputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Terminal::CCloseTerminalSessionResponse OnCloseTerminalSession(
				const sdl::agentino::Terminal::CCloseTerminalSessionGqlRequest& closeTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	I_REF(agentinodata::ITerminalController, m_terminalControllerCompPtr);
};


} // namespace agentgql
