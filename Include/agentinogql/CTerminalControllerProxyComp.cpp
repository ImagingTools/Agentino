// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTerminalControllerProxyComp.h>


// ImtCore includes
#include <imtgql/CGqlContext.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::agentino::Terminal::CGraphQlHandlerCompBase)

sdl::agentino::Terminal::CShellTypeListPayload CTerminalControllerProxyComp::OnListShellTypes(
			const sdl::agentino::Terminal::CListShellTypesGqlRequest& /*listShellTypesRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CShellTypeListPayload retVal =
				SendModelRequest<sdl::agentino::Terminal::CShellTypeListPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::agentino::Terminal::CShellTypeListPayload();
	}

	return retVal;
}


sdl::agentino::Terminal::CTerminalOutputResponse CTerminalControllerProxyComp::OnGetTerminalOutput(
			const sdl::agentino::Terminal::CGetTerminalOutputGqlRequest& /*getTerminalOutputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CTerminalOutputResponse retVal =
				SendModelRequest<sdl::agentino::Terminal::CTerminalOutputResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::agentino::Terminal::CTerminalOutputResponse();
	}

	return retVal;
}


sdl::agentino::Terminal::COpenTerminalSessionResponse CTerminalControllerProxyComp::OnOpenTerminalSession(
			const sdl::agentino::Terminal::COpenTerminalSessionGqlRequest& /*openTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::COpenTerminalSessionResponse retVal =
				SendModelRequest<sdl::agentino::Terminal::COpenTerminalSessionResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::agentino::Terminal::COpenTerminalSessionResponse();
	}

	return retVal;
}


sdl::agentino::Terminal::CSendTerminalInputResponse CTerminalControllerProxyComp::OnSendTerminalInput(
			const sdl::agentino::Terminal::CSendTerminalInputGqlRequest& /*sendTerminalInputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CSendTerminalInputResponse retVal =
				SendModelRequest<sdl::agentino::Terminal::CSendTerminalInputResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::agentino::Terminal::CSendTerminalInputResponse();
	}

	return retVal;
}


sdl::agentino::Terminal::CCloseTerminalSessionResponse CTerminalControllerProxyComp::OnCloseTerminalSession(
			const sdl::agentino::Terminal::CCloseTerminalSessionGqlRequest& /*closeTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CCloseTerminalSessionResponse retVal =
				SendModelRequest<sdl::agentino::Terminal::CCloseTerminalSessionResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::agentino::Terminal::CCloseTerminalSessionResponse();
	}

	return retVal;
}


// reimplemented (imtgql::CGqlRequestHandlerCompBase)

QJsonObject CTerminalControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	if (sdl::agentino::Terminal::CListShellTypesGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Terminal::CListShellTypesGqlRequest,
			sdl::agentino::Terminal::CShellTypeListPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnListShellTypes(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Terminal::CGetTerminalOutputGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Terminal::CGetTerminalOutputGqlRequest,
			sdl::agentino::Terminal::CTerminalOutputResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetTerminalOutput(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Terminal::COpenTerminalSessionGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Terminal::COpenTerminalSessionGqlRequest,
			sdl::agentino::Terminal::COpenTerminalSessionResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnOpenTerminalSession(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Terminal::CSendTerminalInputGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Terminal::CSendTerminalInputGqlRequest,
			sdl::agentino::Terminal::CSendTerminalInputResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnSendTerminalInput(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Terminal::CCloseTerminalSessionGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Terminal::CCloseTerminalSessionGqlRequest,
			sdl::agentino::Terminal::CCloseTerminalSessionResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnCloseTerminalSession(req, gqlReq, err);
			});
	}

	return QJsonObject();
}


// private methods

template<class SdlGqlRequest, class SdlResponse>
QJsonObject CTerminalControllerProxyComp::CreateResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage,
			std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	SdlGqlRequest terminalGqlRequest(gqlRequest, true);

	Q_ASSERT(terminalGqlRequest.IsValid());
	if (!terminalGqlRequest.IsValid()){
		return QJsonObject();
	}

	SdlResponse retVal = func(terminalGqlRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return QJsonObject();
	}

	QJsonObject resultObj;
	if (!retVal.WriteToJsonObject(resultObj)){
		errorMessage = QString("Unable to create response for command '%1'. Error: Writing to JSON object failed").arg(qPrintable(commandId));
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return QJsonObject();
	}

	return resultObj;
}


} // namespace agentinogql
