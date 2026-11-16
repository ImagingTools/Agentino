// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentConnectionTokenControllerComp.h>


// Agentino includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentConnectionTokens.h>


namespace agentgql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CAgentConnectionTokensGqlHandlerCompBase)

sdl::V1_0::agentino::CConnectionTokenState CAgentConnectionTokenControllerComp::OnGetAgentConnectionToken(
			const sdl::V1_0::agentino::CGetAgentConnectionTokenGqlRequest& getAgentConnectionTokenRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenState response;

	const auto arguments = getAgentConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId){
		errorMessage = QStringLiteral("The service and the connection are required");

		return response;
	}

	agentinodata::IConnectionTokenStore::TokenInfo tokenInfo;
	if (!m_tokenStoreCompPtr->GetToken(*arguments.input->serviceId, *arguments.input->connectionId, tokenInfo)){
		return response;
	}

	sdl::V1_0::agentino::CConnectionToken token;
	token.tokenId = tokenInfo.tokenId;
	token.receiverProductId = tokenInfo.receiverProductId;
	token.scopes.Emplace().FromList(tokenInfo.scopes);
	token.issuedAt = tokenInfo.issuedAt.toUTC().toString(Qt::ISODate);
	response.token = token;

	return response;
}


sdl::V1_0::agentino::CConnectionTokenChangePayload CAgentConnectionTokenControllerComp::OnStoreAgentConnectionToken(
			const sdl::V1_0::agentino::CStoreAgentConnectionTokenGqlRequest& storeAgentConnectionTokenRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenChangePayload response;
	response.success = false;

	const auto arguments = storeAgentConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->token){
		errorMessage = QStringLiteral("The service, the connection and the token are required");

		return response;
	}

	const sdl::V1_0::agentino::CConnectionToken& token = *arguments.input->token;

	agentinodata::IConnectionTokenStore::TokenInfo tokenInfo;
	tokenInfo.tokenId = token.tokenId ? QByteArray(*token.tokenId) : QByteArray();
	tokenInfo.receiverProductId = token.receiverProductId ? QByteArray(*token.receiverProductId) : QByteArray();
	tokenInfo.scopes = token.scopes ? token.scopes->ToList() : QByteArrayList();
	tokenInfo.issuedAt = token.issuedAt ? QDateTime::fromString(*token.issuedAt, Qt::ISODate) : QDateTime::currentDateTimeUtc();

	const QByteArray value = arguments.input->value ? QByteArray(*arguments.input->value) : QByteArray();

	const QByteArray serviceId = *arguments.input->serviceId;
	response.success = m_tokenStoreCompPtr->StoreToken(serviceId, *arguments.input->connectionId, tokenInfo, value);
	if (!*response.success){
		errorMessage = QStringLiteral("Unable to keep the token of the connection");

		return response;
	}

	// A running service gets a new token only with its environment at the start: restart it; missing when it does not run
	if (!value.isEmpty() && m_serviceControllerCompPtr.IsValid()){
		const agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceControllerCompPtr->GetServiceStatus(serviceId);
		if (status == agentinodata::IServiceStatusInfo::SS_RUNNING || status == agentinodata::IServiceStatusInfo::SS_STARTING){
			response.restarted = m_serviceControllerCompPtr->RestartService(serviceId);
		}
	}

	return response;
}


sdl::V1_0::agentino::CConnectionTokenChangePayload CAgentConnectionTokenControllerComp::OnRemoveAgentConnectionToken(
			const sdl::V1_0::agentino::CRemoveAgentConnectionTokenGqlRequest& removeAgentConnectionTokenRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenChangePayload response;
	response.success = false;

	const auto arguments = removeAgentConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId){
		errorMessage = QStringLiteral("The service and the connection are required");

		return response;
	}

	response.success = m_tokenStoreCompPtr->RemoveToken(*arguments.input->serviceId, *arguments.input->connectionId);
	if (!*response.success){
		errorMessage = QStringLiteral("Unable to remove the token of the connection");
	}

	return response;
}


} // namespace agentgql


