// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTerminalControllerProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal.h>


// ImtCore includes
#include <imtgql/CGqlRequest.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CTerminalGqlHandlerCompBase)

sdl::V1_0::agentino::CShellTypeListPayload CTerminalControllerProxyComp::OnListShellTypes(
			const sdl::V1_0::agentino::CListShellTypesGqlRequest& /*listShellTypesRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::CShellTypeListPayload>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CTerminalOutputResponse CTerminalControllerProxyComp::OnGetTerminalOutput(
			const sdl::V1_0::agentino::CGetTerminalOutputGqlRequest& /*getTerminalOutputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::CTerminalOutputResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::COpenTerminalSessionResponse CTerminalControllerProxyComp::OnOpenTerminalSession(
			const sdl::V1_0::agentino::COpenTerminalSessionGqlRequest& /*openTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::COpenTerminalSessionResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CSendTerminalInputResponse CTerminalControllerProxyComp::OnSendTerminalInput(
			const sdl::V1_0::agentino::CSendTerminalInputGqlRequest& /*sendTerminalInputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::CSendTerminalInputResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CInterruptTerminalSessionResponse CTerminalControllerProxyComp::OnInterruptTerminalSession(
			const sdl::V1_0::agentino::CInterruptTerminalSessionGqlRequest& /*interruptTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::CInterruptTerminalSessionResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CCloseTerminalSessionResponse CTerminalControllerProxyComp::OnCloseTerminalSession(
			const sdl::V1_0::agentino::CCloseTerminalSessionGqlRequest& /*closeTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	return ForwardToAgent<sdl::V1_0::agentino::CCloseTerminalSessionResponse>(gqlRequest, errorMessage);
}


// private methods

template <class SdlResponse>
SdlResponse CTerminalControllerProxyComp::ForwardToAgent(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	// Require clientid so the shell is opened on a concrete agent instead of silently
	// hitting whatever the API client happens to be connected to.
	if (gqlRequest.GetHeader(QByteArrayLiteral("clientid")).isEmpty()){
		errorMessage = QStringLiteral(
					"Unable to serve terminal request. Error: request header 'clientid' (agent id) is missing");
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return SdlResponse();
	}

	// Permission check is performed by CPermissibleGqlRequestHandlerComp base
	// (CreateResponse -> CheckPermissions) before this method is invoked.
	SdlResponse retVal = SendModelRequest<SdlResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return SdlResponse();
	}

	return retVal;
}


} // namespace agentinogql
