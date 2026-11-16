// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMutex>

// ImtCore includes
#include <imtauthgql/IServerTokenIssuer.h>


namespace agentinogql
{


/**
	Superuser sessions at receiving servers, kept on this server between the sign-in and the change of a token.
	The operator gets an ID of the sign-in only; it works once, for its operator and its connection.
*/
class CReceiverSessionPool
{
public:
	struct Session
	{
		QByteArray sessionToken;
		imtauthgql::IServerTokenIssuer::ServerInfo server;

		/**
			The operator of this server who signed in.
		*/
		QByteArray operatorId;
		QByteArray agentId;
		QByteArray serviceId;
		QByteArray connectionId;
		QDateTime expiresAt;
	};

	/**
		\return the ID of the sign-in.
	*/
	QByteArray Add(const Session& session);

	/**
		Remove the session of the sign-in; an expired one is not returned.
	*/
	bool Take(const QByteArray& signInId, Session& session);

	/**
		Remove the expired sessions; the caller ends them at their servers.
	*/
	QList<Session> TakeExpired();

private:
	QMutex m_mutex;
	QMap<QByteArray, Session> m_sessions;
};


} // namespace agentinogql


