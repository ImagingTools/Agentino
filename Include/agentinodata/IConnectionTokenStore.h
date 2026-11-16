// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArrayList>
#include <QtCore/QDateTime>

// ACF includes
#include <istd/IPolymorphic.h>


namespace agentinodata
{


/**
	The tokens the output connections of the services hold at the servers they connect to.
	The server at the other end issues a token; the store keeps its value protected for the service.
*/
class IConnectionTokenStore: virtual public istd::IPolymorphic
{
public:
	struct TokenInfo
	{
		QByteArray tokenId;

		/**
			The product of the server that issued the token and takes it.
		*/
		QByteArray receiverProductId;

		QByteArrayList scopes;
		QDateTime issuedAt;
	};

	/**
		\return false if the connection holds no token.
	*/
	virtual bool GetToken(const QByteArray& serviceId, const QByteArray& connectionId, TokenInfo& tokenInfo) const = 0;

	/**
		Store the token of a connection.
		\param value	The secret of the token; empty keeps the stored one.
	*/
	virtual bool StoreToken(const QByteArray& serviceId, const QByteArray& connectionId, const TokenInfo& tokenInfo, const QByteArray& value) = 0;

	virtual bool RemoveToken(const QByteArray& serviceId, const QByteArray& connectionId) = 0;
};


} // namespace agentinodata


