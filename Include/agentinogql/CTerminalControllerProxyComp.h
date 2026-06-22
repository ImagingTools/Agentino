// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QJsonObject>

// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal.h>


namespace agentinogql
{


/**
	Server side proxy for the remote terminal feature.

	Runs inside the AgentinoServer process and simply forwards every terminal request to
	the agent selected through the 'clientid' request header, returning the agent response
	unchanged. The actual shell process always runs on the agent machine.

	\ingroup Terminal
*/
class CTerminalControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
										sdl::agentino::Terminal::CGraphQlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::agentino::Terminal::CGraphQlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerProxyComp);
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

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual QJsonObject CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	template<class SdlGqlRequest, class SdlResponse>
	QJsonObject CreateResponse(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage,
				std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const;
};


} // namespace agentinogql
