// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtauthgql/IServerTokenIssuer.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/ConnectionTokens.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentConnectionTokens.h>


namespace agentinogql
{


/**
	The tokens of the output connections of the services. The server a connection goes to issues the token for itself,
	with a superuser session the operator opens there; the agent of the service keeps the value and passes it to the service.
*/
class CConnectionTokenControllerComp:
			public imtclientgql::TClientRequestManagerCompWrap<sdl::V1_0::agentino::CConnectionTokensGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<sdl::V1_0::agentino::CConnectionTokensGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CConnectionTokenControllerComp);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "The services of the agents", true, "ServiceManager");
		I_ASSIGN(m_tokenIssuerCompPtr, "TokenIssuer", "Tokens of the servers the connections go to", true, "TokenIssuer");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CConnectionTokensGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CConnectionTokenState OnGetConnectionToken(
				const sdl::V1_0::agentino::CGetConnectionTokenGqlRequest& getConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CConnectionTokenCatalog OnGetConnectionTokenCatalog(
				const sdl::V1_0::agentino::CGetConnectionTokenCatalogGqlRequest& getConnectionTokenCatalogRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSetConnectionTokenPayload OnSetConnectionToken(
				const sdl::V1_0::agentino::CSetConnectionTokenGqlRequest& setConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CRevokeConnectionTokenPayload OnRevokeConnectionToken(
				const sdl::V1_0::agentino::CRevokeConnectionTokenGqlRequest& revokeConnectionTokenRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		An output connection and the server it goes to.
	*/
	struct Connection
	{
		QByteArray agentId;
		QByteArray serviceId;
		QByteArray connectionId;

		/**
			The product of the service: the token is issued for it.
		*/
		QByteArray clientProductId;

		QString tokenName;
		imtauthgql::IServerTokenIssuer::ServerInfo server;
	};

	bool FindConnection(
				const ::imtgql::CGqlRequest& gqlRequest,
				const QByteArray& serviceId,
				const QByteArray& connectionId,
				Connection& connection,
				QString& errorMessage) const;

	/**
		\return false if the agent could not be asked; a connection without a token has an empty state.
	*/
	bool GetStoredToken(const Connection& connection, sdl::V1_0::agentino::CConnectionTokenState& state, QString& errorMessage) const;
	bool StoreToken(const Connection& connection, const sdl::V1_0::agentino::CConnectionToken& token, const QByteArray& value, QString& errorMessage) const;
	bool RemoveStoredToken(const Connection& connection, QString& errorMessage) const;

	template<class GqlRequest, class Payload, class Arguments>
	Payload SendToAgent(const Connection& connection, const Arguments& arguments, QString& errorMessage) const;

private:
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(imtauthgql::IServerTokenIssuer, m_tokenIssuerCompPtr);
};


} // namespace agentinogql


