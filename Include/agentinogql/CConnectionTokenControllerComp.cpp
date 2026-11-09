// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CConnectionTokenControllerComp.h>


// Qt includes
#include <QtCore/QDateTime>

// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtgql/CGqlContext.h>
#include <imtservice/IServiceConnectionLinkParam.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CConnectionTokensGqlHandlerCompBase)

sdl::V1_0::agentino::CConnectionTokenState CConnectionTokenControllerComp::OnGetConnectionToken(
			const sdl::V1_0::agentino::CGetConnectionTokenGqlRequest& getConnectionTokenRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenState response;

	const auto arguments = getConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId){
		errorMessage = QStringLiteral("The service and the connection are required");

		return response;
	}

	Connection connection;
	connection.agentId = gqlRequest.GetHeader("clientid");
	connection.serviceId = *arguments.input->serviceId;
	connection.connectionId = *arguments.input->connectionId;

	GetStoredToken(connection, response, errorMessage);

	return response;
}


sdl::V1_0::agentino::CConnectionTokenCatalog CConnectionTokenControllerComp::OnGetConnectionTokenCatalog(
			const sdl::V1_0::agentino::CGetConnectionTokenCatalogGqlRequest& getConnectionTokenCatalogRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenCatalog response;

	const auto arguments = getConnectionTokenCatalogRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->password){
		errorMessage = QStringLiteral("The service, the connection and the password of the superuser are required");

		return response;
	}

	Connection connection;
	if (!FindConnection(gqlRequest, *arguments.input->serviceId, *arguments.input->connectionId, connection, errorMessage)){
		return response;
	}

	const QByteArray sessionToken = m_tokenIssuerCompPtr->SignInSuperuser(connection.server, *arguments.input->password, errorMessage);
	if (sessionToken.isEmpty()){
		return response;
	}

	// The server issues tokens to its clients only: the product of the service becomes one
	bool isClientAdded = false;
	if (!m_tokenIssuerCompPtr->AddClient(connection.server, sessionToken, connection.clientProductId, QString::fromUtf8(connection.clientProductId), isClientAdded, errorMessage)){
		return response;
	}

	sdl::V1_0::imtauth::CServerTokenReceiver permissions;
	if (!m_tokenIssuerCompPtr->GetPermissions(connection.server, sessionToken, connection.clientProductId, permissions, errorMessage)){
		return response;
	}

	sdl::V1_0::agentino::CConnectionTokenState state;
	if (!GetStoredToken(connection, state, errorMessage)){
		return response;
	}

	response.sessionToken = sessionToken;
	response.receiverProductId = connection.server.productId;
	response.groups = permissions.groups;
	response.requiredPermissions = permissions.requiredPermissions;
	response.token = state.token;
	response.clientAdded = isClientAdded;

	return response;
}


sdl::V1_0::agentino::CSetConnectionTokenPayload CConnectionTokenControllerComp::OnSetConnectionToken(
			const sdl::V1_0::agentino::CSetConnectionTokenGqlRequest& setConnectionTokenRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSetConnectionTokenPayload response;

	const auto arguments = setConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->sessionToken){
		errorMessage = QStringLiteral("The service, the connection and the session are required");

		return response;
	}

	Connection connection;
	if (!FindConnection(gqlRequest, *arguments.input->serviceId, *arguments.input->connectionId, connection, errorMessage)){
		return response;
	}

	sdl::V1_0::agentino::CConnectionTokenState state;
	if (!GetStoredToken(connection, state, errorMessage)){
		return response;
	}

	const QByteArray sessionToken = *arguments.input->sessionToken;
	QByteArrayList scopes = arguments.input->scopes ? arguments.input->scopes->ToList() : QByteArrayList();

	sdl::V1_0::agentino::CConnectionToken token;
	token.receiverProductId = connection.server.productId;

	// A token of the same server keeps its value: only its permissions change, the service needs no restart
	const bool isHeld = state.token && state.token->tokenId && state.token->receiverProductId
				&& QByteArray(*state.token->receiverProductId) == connection.server.productId;
	if (isHeld){
		if (!m_tokenIssuerCompPtr->SetTokenScopes(connection.server, sessionToken, *state.token->tokenId, scopes, errorMessage)){
			return response;
		}

		token.tokenId = state.token->tokenId;
		token.issuedAt = state.token->issuedAt;
		token.scopes.Emplace().FromList(scopes);
		if (!StoreToken(connection, token, QByteArray(), errorMessage)){
			return response;
		}

		response.token = token;
		response.restartRequired = false;

		return response;
	}

	QByteArray tokenId;
	QByteArray value;
	if (!m_tokenIssuerCompPtr->IssueToken(connection.server, sessionToken, connection.clientProductId, connection.tokenName, scopes, tokenId, value, errorMessage)){
		return response;
	}

	token.tokenId = tokenId;
	token.issuedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	token.scopes.Emplace().FromList(scopes);
	if (!StoreToken(connection, token, value, errorMessage)){
		// A token nobody holds must not stay valid
		QString revokeError;
		m_tokenIssuerCompPtr->RevokeToken(connection.server, sessionToken, tokenId, revokeError);

		return response;
	}

	response.token = token;
	response.restartRequired = true;

	return response;
}


sdl::V1_0::agentino::CRevokeConnectionTokenPayload CConnectionTokenControllerComp::OnRevokeConnectionToken(
			const sdl::V1_0::agentino::CRevokeConnectionTokenGqlRequest& revokeConnectionTokenRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CRevokeConnectionTokenPayload response;
	response.success = false;

	const auto arguments = revokeConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->password){
		errorMessage = QStringLiteral("The service, the connection and the password of the superuser are required");

		return response;
	}

	Connection connection;
	if (!FindConnection(gqlRequest, *arguments.input->serviceId, *arguments.input->connectionId, connection, errorMessage)){
		return response;
	}

	sdl::V1_0::agentino::CConnectionTokenState state;
	if (!GetStoredToken(connection, state, errorMessage)){
		return response;
	}

	if (state.token && state.token->tokenId){
		const QByteArray sessionToken = m_tokenIssuerCompPtr->SignInSuperuser(connection.server, *arguments.input->password, errorMessage);
		if (sessionToken.isEmpty() || !m_tokenIssuerCompPtr->RevokeToken(connection.server, sessionToken, *state.token->tokenId, errorMessage)){
			return response;
		}
	}

	response.success = RemoveStoredToken(connection, errorMessage);

	return response;
}


// private methods

bool CConnectionTokenControllerComp::FindConnection(
			const ::imtgql::CGqlRequest& gqlRequest,
			const QByteArray& serviceId,
			const QByteArray& connectionId,
			Connection& connection,
			QString& errorMessage) const
{
	connection.agentId = gqlRequest.GetHeader("clientid");
	connection.serviceId = serviceId;
	connection.connectionId = connectionId;

	istd::TDelPtr<agentinodata::IServiceInfo> serviceInfoPtr(m_serviceManagerCompPtr->GetService(connection.agentId, serviceId));
	if (!serviceInfoPtr.IsValid()){
		errorMessage = QStringLiteral("The service '%1' of agent '%2' is unknown").arg(QString::fromUtf8(serviceId), QString::fromUtf8(connection.agentId));

		return false;
	}

	imtbase::IObjectCollection* linksPtr = serviceInfoPtr->GetDependantServiceConnections();
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (linksPtr == nullptr || !linksPtr->GetObjectData(connectionId, dataPtr)){
		errorMessage = QStringLiteral("The service has no output connection '%1'").arg(QString::fromUtf8(connectionId));

		return false;
	}

	const imtservice::IServiceConnectionLinkParam* linkPtr = dynamic_cast<const imtservice::IServiceConnectionLinkParam*>(dataPtr.GetPtr());
	const imtcom::IServerConnectionInterface* addressPtr = dynamic_cast<const imtcom::IServerConnectionInterface*>(dataPtr.GetPtr());
	if (linkPtr == nullptr || addressPtr == nullptr || linkPtr->GetDependantServiceConnectionId().isEmpty()){
		errorMessage = QStringLiteral("Link the connection to a server first");

		return false;
	}

	// The servers of the products answer GraphQL under their product ID
	connection.server.productId = linkPtr->GetServiceTypeId();
	if (!addressPtr->GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, connection.server.url)){
		errorMessage = QStringLiteral("The server the connection goes to has no HTTP address");

		return false;
	}
	connection.server.url.setPath(QStringLiteral("/%1/graphql").arg(QString::fromUtf8(connection.server.productId)));

	connection.clientProductId = serviceInfoPtr->GetServiceTypeId().toUtf8();
	connection.tokenName = QStringLiteral("%1 (%2)").arg(serviceInfoPtr->GetServiceName(), QString::fromUtf8(connection.agentId));

	return true;
}


bool CConnectionTokenControllerComp::GetStoredToken(const Connection& connection, sdl::V1_0::agentino::CConnectionTokenState& state, QString& errorMessage) const
{
	sdl::V1_0::agentino::GetAgentConnectionTokenRequestArguments arguments;
	arguments.input.emplace();
	arguments.input->serviceId = connection.serviceId;
	arguments.input->connectionId = connection.connectionId;

	state = SendToAgent<sdl::V1_0::agentino::CGetAgentConnectionTokenGqlRequest, sdl::V1_0::agentino::CConnectionTokenState>(connection, arguments, errorMessage);

	return errorMessage.isEmpty();
}


bool CConnectionTokenControllerComp::StoreToken(const Connection& connection, const sdl::V1_0::agentino::CConnectionToken& token, const QByteArray& value, QString& errorMessage) const
{
	sdl::V1_0::agentino::StoreAgentConnectionTokenRequestArguments arguments;
	arguments.input.emplace();
	arguments.input->serviceId = connection.serviceId;
	arguments.input->connectionId = connection.connectionId;
	arguments.input->token = token;
	if (!value.isEmpty()){
		arguments.input->value = value;
	}

	const sdl::V1_0::agentino::CConnectionTokenChangePayload payload =
				SendToAgent<sdl::V1_0::agentino::CStoreAgentConnectionTokenGqlRequest, sdl::V1_0::agentino::CConnectionTokenChangePayload>(connection, arguments, errorMessage);

	return errorMessage.isEmpty() && payload.success && *payload.success;
}


bool CConnectionTokenControllerComp::RemoveStoredToken(const Connection& connection, QString& errorMessage) const
{
	sdl::V1_0::agentino::RemoveAgentConnectionTokenRequestArguments arguments;
	arguments.input.emplace();
	arguments.input->serviceId = connection.serviceId;
	arguments.input->connectionId = connection.connectionId;

	const sdl::V1_0::agentino::CConnectionTokenChangePayload payload =
				SendToAgent<sdl::V1_0::agentino::CRemoveAgentConnectionTokenGqlRequest, sdl::V1_0::agentino::CConnectionTokenChangePayload>(connection, arguments, errorMessage);

	return errorMessage.isEmpty() && payload.success && *payload.success;
}


template<class GqlRequest, class Payload, class Arguments>
Payload CConnectionTokenControllerComp::SendToAgent(const Connection& connection, const Arguments& arguments, QString& errorMessage) const
{
	// The agent of the service is addressed by its client ID
	imtgql::CGqlRequest gqlRequest;
	if (!GqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		errorMessage = QStringLiteral("Unable to build the request to the agent");

		return Payload();
	}

	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", connection.agentId);
	imtgql::IGqlContextUniquePtr contextPtr(new imtgql::CGqlContext);
	contextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(imtgql::IGqlContextSharedPtr::CreateFromUnique(std::move(contextPtr)));

	return SendModelRequest<Payload>(gqlRequest, errorMessage);
}


} // namespace agentinogql


