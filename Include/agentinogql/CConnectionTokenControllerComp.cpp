// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CConnectionTokenControllerComp.h>


// Qt includes
#include <QtCore/QDateTime>

// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtauthgql/imtauthgql.h>
#include <imtgql/CGqlContext.h>
#include <imtservice/IServiceConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/ServiceEndpointId.h>


namespace agentinogql
{


namespace
{


// The operator chooses the permissions within this time after the sign-in
const int s_signInLifetimeSecs = 10 * 60;


} // namespace


// protected methods

// reimplemented (sdl::V1_0::agentino::CConnectionTokensGqlHandlerCompBase)

sdl::V1_0::agentino::CConnectionTokenState CConnectionTokenControllerComp::OnGetConnectionToken(
			const sdl::V1_0::agentino::CGetConnectionTokenGqlRequest& getConnectionTokenRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenState response;

	if (!CheckOperator(gqlRequest, errorMessage)){
		return response;
	}

	const auto arguments = getConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId){
		errorMessage = QStringLiteral("The service and the connection are required");

		return response;
	}

	Connection connection;
	connection.agentId = gqlRequest.GetHeader("clientid");
	connection.serviceId = *arguments.input->serviceId;
	connection.connectionId = *arguments.input->connectionId;

	if (!GetStoredToken(connection, response, errorMessage)){
		return response;
	}

	// Whether a token can be issued here, and why not
	Connection target;
	QString refusal;
	response.canIssue = FindConnection(gqlRequest, connection.serviceId, connection.connectionId, target, refusal) && CheckReceiver(target, refusal);
	if (!refusal.isEmpty()){
		response.cannotIssueReason = refusal;
	}

	return response;
}


sdl::V1_0::agentino::CConnectionTokenCatalog CConnectionTokenControllerComp::OnGetConnectionTokenCatalog(
			const sdl::V1_0::agentino::CGetConnectionTokenCatalogGqlRequest& getConnectionTokenCatalogRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CConnectionTokenCatalog response;

	if (!CheckOperator(gqlRequest, errorMessage)){
		return response;
	}

	const auto arguments = getConnectionTokenCatalogRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->password){
		errorMessage = QStringLiteral("The service, the connection and the password of the superuser are required");

		return response;
	}

	Connection connection;
	if (!FindConnection(gqlRequest, *arguments.input->serviceId, *arguments.input->connectionId, connection, errorMessage) || !CheckReceiver(connection, errorMessage)){
		return response;
	}

	const QByteArray sessionToken = m_tokenIssuerCompPtr->SignInSuperuser(connection.server, *arguments.input->password, errorMessage);
	if (sessionToken.isEmpty()){
		return response;
	}

	// The server issues tokens to its clients only: the product of the service becomes one
	bool isClientAdded = false;
	sdl::V1_0::imtauth::CServerTokenReceiver permissions;
	sdl::V1_0::agentino::CConnectionTokenState state;
	if (!m_tokenIssuerCompPtr->AddClient(connection.server, sessionToken, connection.clientProductId, QString::fromUtf8(connection.clientProductId), isClientAdded, errorMessage)
				|| !m_tokenIssuerCompPtr->GetPermissions(connection.server, sessionToken, connection.clientProductId, permissions, errorMessage)
				|| !GetStoredToken(connection, state, errorMessage)){
		EndSession(connection.server, sessionToken);

		return response;
	}

	CReceiverSessionPool::Session session;
	session.sessionToken = sessionToken;
	session.server = connection.server;
	session.operatorId = gqlRequest.GetRequestContext()->GetUserId();
	session.agentId = connection.agentId;
	session.serviceId = connection.serviceId;
	session.connectionId = connection.connectionId;
	session.expiresAt = QDateTime::currentDateTimeUtc().addSecs(s_signInLifetimeSecs);

	response.signInId = m_sessionPool.Add(session);
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

	if (!CheckOperator(gqlRequest, errorMessage)){
		return response;
	}

	const auto arguments = setConnectionTokenRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->serviceId || !arguments.input->connectionId || !arguments.input->signInId){
		errorMessage = QStringLiteral("The service, the connection and the sign-in are required");

		return response;
	}

	CReceiverSessionPool::Session session;
	if (!m_sessionPool.Take(*arguments.input->signInId, session)){
		errorMessage = QStringLiteral("The sign-in has expired or has been used, sign in again");

		return response;
	}

	// One session, one change of the token: it ends whatever the change gives
	const bool isOwnSignIn = session.operatorId == gqlRequest.GetRequestContext()->GetUserId()
				&& session.agentId == gqlRequest.GetHeader("clientid")
				&& session.serviceId == *arguments.input->serviceId
				&& session.connectionId == *arguments.input->connectionId;
	if (!isOwnSignIn){
		EndSession(session.server, session.sessionToken);
		errorMessage = QStringLiteral("The sign-in belongs to another operator or connection");

		return response;
	}

	ChangeToken(session, arguments.input->scopes ? arguments.input->scopes->ToList() : QByteArrayList(), gqlRequest, response, errorMessage);
	EndSession(session.server, session.sessionToken);

	return response;
}


sdl::V1_0::agentino::CRevokeConnectionTokenPayload CConnectionTokenControllerComp::OnRevokeConnectionToken(
			const sdl::V1_0::agentino::CRevokeConnectionTokenGqlRequest& revokeConnectionTokenRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CRevokeConnectionTokenPayload response;
	response.success = false;

	if (!CheckOperator(gqlRequest, errorMessage)){
		return response;
	}

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
		if (sessionToken.isEmpty()){
			return response;
		}

		const bool isRevoked = m_tokenIssuerCompPtr->RevokeToken(connection.server, sessionToken, *state.token->tokenId, errorMessage);
		EndSession(connection.server, sessionToken);
		if (!isRevoked){
			return response;
		}
	}

	response.success = RemoveStoredToken(connection, errorMessage);

	return response;
}


sdl::V1_0::agentino::CCancelConnectionTokenSignInPayload CConnectionTokenControllerComp::OnCancelConnectionTokenSignIn(
			const sdl::V1_0::agentino::CCancelConnectionTokenSignInGqlRequest& cancelConnectionTokenSignInRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CCancelConnectionTokenSignInPayload response;
	response.success = false;

	if (!CheckOperator(gqlRequest, errorMessage)){
		return response;
	}

	const auto arguments = cancelConnectionTokenSignInRequest.GetRequestedArguments();
	CReceiverSessionPool::Session session;
	if (!arguments.input || !arguments.input->signInId || !m_sessionPool.Take(*arguments.input->signInId, session)){
		errorMessage = QStringLiteral("The sign-in has expired or has been used, sign in again");

		return response;
	}

	// The session ends whoever cancels: a sign-in ID in other hands is spent as well
	EndSession(session.server, session.sessionToken);
	response.success = true;

	return response;
}


// private methods

bool CConnectionTokenControllerComp::CheckOperator(const ::imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	for (const CReceiverSessionPool::Session& session : m_sessionPool.TakeExpired()){
		EndSession(session.server, session.sessionToken);
	}

	if (!imtauthgql::IsSuperuserSessionRequest(gqlRequest)){
		errorMessage = QStringLiteral("Connection tokens are managed by a superuser session of Agentino");

		return false;
	}

	return true;
}


void CConnectionTokenControllerComp::EndSession(const imtauthgql::IServerTokenIssuer::ServerInfo& server, const QByteArray& sessionToken) const
{
	QString errorMessage;
	if (!m_tokenIssuerCompPtr->SignOut(server, sessionToken, errorMessage)){
		SendWarningMessage(0, QStringLiteral("The session at '%1' did not end: %2").arg(QString::fromUtf8(server.productId), errorMessage), "CConnectionTokenControllerComp");
	}
}


bool CConnectionTokenControllerComp::ChangeToken(
			const CReceiverSessionPool::Session& session,
			QByteArrayList scopes,
			const ::imtgql::CGqlRequest& gqlRequest,
			sdl::V1_0::agentino::CSetConnectionTokenPayload& response,
			QString& errorMessage) const
{
	Connection connection;
	if (!FindConnection(gqlRequest, session.serviceId, session.connectionId, connection, errorMessage) || !CheckReceiver(connection, errorMessage)){
		return false;
	}

	// The token is changed where the operator signed in
	connection.server = session.server;
	const QByteArray& sessionToken = session.sessionToken;

	sdl::V1_0::agentino::CConnectionTokenState state;
	if (!GetStoredToken(connection, state, errorMessage)){
		return false;
	}

	sdl::V1_0::agentino::CConnectionToken token;
	token.receiverProductId = connection.server.productId;

	// A token of the same server keeps its value: only its permissions change, the service needs no restart
	const bool isHeld = state.token && state.token->tokenId && state.token->receiverProductId
				&& QByteArray(*state.token->receiverProductId) == connection.server.productId;
	if (isHeld){
		if (!m_tokenIssuerCompPtr->SetTokenScopes(connection.server, sessionToken, *state.token->tokenId, scopes, errorMessage)){
			return false;
		}

		token.tokenId = state.token->tokenId;
		token.issuedAt = state.token->issuedAt;
		token.scopes.Emplace().FromList(scopes);
		if (!StoreToken(connection, token, QByteArray(), errorMessage)){
			return false;
		}

		response.token = token;
		response.restartRequired = false;

		return true;
	}

	QByteArray tokenId;
	QByteArray value;
	if (!m_tokenIssuerCompPtr->IssueToken(connection.server, sessionToken, connection.clientProductId, connection.tokenName, scopes, tokenId, value, errorMessage)){
		return false;
	}

	token.tokenId = tokenId;
	token.issuedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	token.scopes.Emplace().FromList(scopes);
	if (!StoreToken(connection, token, value, errorMessage)){
		// A token nobody holds must not stay valid
		QString revokeError;
		m_tokenIssuerCompPtr->RevokeToken(connection.server, sessionToken, tokenId, revokeError);

		return false;
	}

	response.token = token;
	response.restartRequired = true;

	return true;
}


bool CConnectionTokenControllerComp::CheckReceiver(const Connection& connection, QString& errorMessage) const
{
	QByteArray serviceId;
	QByteArray connectionId;
	const QByteArray agentId = agentinodata::ServiceEndpointId::Parse(connection.receiverEndpointId, serviceId, connectionId)
				? m_serviceManagerCompPtr->FindServiceAgent(serviceId)
				: QByteArray();
	if (agentId.isEmpty()){
		errorMessage = QStringLiteral("The server the connection goes to is not known: its agent is not connected");

		return false;
	}

	// Only a service whose plugin declares the input receives server tokens
	istd::TDelPtr<agentinodata::IServiceInfo> receiverPtr(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (!receiverPtr.IsValid() || !receiverPtr->IsServerTokenReceiver(connectionId)){
		errorMessage = QStringLiteral("The server the connection goes to does not receive server tokens");

		return false;
	}

	return true;
}


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

	connection.receiverEndpointId = linkPtr->GetDependantServiceConnectionId();

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


