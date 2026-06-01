// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CAgentLocalTopologyProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology.h>


// Qt includes
#include <QtCore/QJsonObject>

// ACF includes
#include <imtgql/CGqlContext.h>


namespace agentinogql
{


// reimplemented (agentinogql::IAgentLocalTopologyProxy)

sdl::V1_0::agentino::CLocalTopology CAgentLocalTopologyProxyComp::QueryLocalTopology(
			const QByteArray& agentId,
			QString& errorMessage) const
{
	namespace toplogy = sdl::V1_0::agentino;

	toplogy::GetLocalTopologyRequestArguments arguments;

	imtgql::CGqlRequest gqlRequest;

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!toplogy::CGetLocalTopologyGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		errorMessage = QString("Unable to query local topology for agent '%1'. Error: Setup GraphQL request failed").arg(qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CAgentLocalTopologyProxyComp");

		return toplogy::CLocalTopology();
	}

	toplogy::CLocalTopology response = SendModelRequest<toplogy::CLocalTopology>(gqlRequest, errorMessage);

	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, QString("Unable to query local topology for agent '%1'. Error: %2").arg(qPrintable(agentId), errorMessage), "CAgentLocalTopologyProxyComp");
	}

	return response;
}


bool CAgentLocalTopologyProxyComp::PushLocalTopology(
			const QByteArray& agentId,
			const sdl::V1_0::agentino::CLocalTopology& topology,
			QString& errorMessage) const
{
	namespace toplogy = sdl::V1_0::agentino;

	toplogy::SaveLocalTopologyRequestArguments arguments;
	arguments.input = topology;

	imtgql::CGqlRequest gqlRequest;

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!toplogy::CSaveLocalTopologyGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		errorMessage = QString("Unable to push local topology to agent '%1'. Error: Setup GraphQL request failed").arg(qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CAgentLocalTopologyProxyComp");

		return false;
	}

	toplogy::CSaveLocalTopologyResponse response = SendModelRequest<toplogy::CSaveLocalTopologyResponse>(gqlRequest, errorMessage);

	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, QString("Unable to push local topology to agent '%1'. Error: %2").arg(qPrintable(agentId), errorMessage), "CAgentLocalTopologyProxyComp");

		return false;
	}

	return response.successful.has_value() && *response.successful;
}


// reimplemented (sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase)
// Not used on server side; stubs to satisfy abstract base.

sdl::V1_0::agentino::CLocalTopology CAgentLocalTopologyProxyComp::OnGetLocalTopology(
			const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& /*getLocalTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	return sdl::V1_0::agentino::CLocalTopology();
}


sdl::V1_0::agentino::CSaveLocalTopologyResponse CAgentLocalTopologyProxyComp::OnSaveLocalTopology(
			const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& /*saveLocalTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	return sdl::V1_0::agentino::CSaveLocalTopologyResponse();
}


// reimplemented (imtgql::CGqlRequestHandlerCompBase)

QJsonObject CAgentLocalTopologyProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	const QByteArray commandId = gqlRequest.GetCommandId();

	if (sdl::V1_0::agentino::CGetLocalTopologyGqlRequest::GetCommandId() == commandId){
		return CreateProxyResponse<
			sdl::V1_0::agentino::CGetLocalTopologyGqlRequest,
			sdl::V1_0::agentino::CLocalTopology>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetLocalTopology(req, gqlReq, err);
			});
	}

	if (sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest::GetCommandId() == commandId){
		return CreateProxyResponse<
			sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest,
			sdl::V1_0::agentino::CSaveLocalTopologyResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnSaveLocalTopology(req, gqlReq, err);
			});
	}

	return QJsonObject();
}


// private methods

template<class SdlGqlRequest, class SdlResponse>
QJsonObject CAgentLocalTopologyProxyComp::CreateProxyResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage,
			std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const
{
	const QByteArray commandId = gqlRequest.GetCommandId();

	SdlGqlRequest typedRequest(gqlRequest, true);

	Q_ASSERT(typedRequest.IsValid());
	if (!typedRequest.IsValid()){
		return QJsonObject();
	}

	SdlResponse retVal = func(typedRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CAgentLocalTopologyProxyComp");

		return QJsonObject();
	}

	QJsonObject resultObj;
	if (!retVal.WriteToJsonObject(resultObj)){
		errorMessage = QString("Unable to create response for command '%1'. Error: Writing to JSON object failed").arg(qPrintable(commandId));
		SendErrorMessage(0, errorMessage, "CAgentLocalTopologyProxyComp");

		return QJsonObject();
	}

	return resultObj;
}


} // namespace agentinogql
