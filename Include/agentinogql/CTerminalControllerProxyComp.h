// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal_fwd.h>


namespace agentinogql
{


/**
	Server side proxy for the remote terminal feature.

	Runs inside the AgentinoServer process and forwards every terminal request to the
	agent identified by the request header \c clientid (same routing channel as
	\ref CServiceControllerProxyComp and \ref CFileSystemControllerProxyComp), returning
	the agent response unchanged. The actual shell process always runs on the agent
	machine.

	\ingroup Terminal
*/
class CTerminalControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
										sdl::V1_0::agentino::CTerminalGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::V1_0::agentino::CTerminalGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerProxyComp);
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

private:
	/**
		Forward the request to the agent addressed by the \c clientid header.
		\return The agent response, or a default constructed one when the header is
		missing or the agent answered with an error.
	*/
	template <class SdlResponse>
	SdlResponse ForwardToAgent(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;
};


} // namespace agentinogql
