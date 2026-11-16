// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>

// ACF includes
#include <ifile/IFileNameParam.h>
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtcrypt/ISecretProtector.h>

// Agentino includes
#include <agentinodata/IConnectionTokenStore.h>
#include <agentinodata/IServiceEnvironmentProvider.h>


namespace agentinodata
{


/**
	The tokens of the output connections in a file readable by the account of the agent only, their values protected.
	A service gets the tokens of its linked output connections, each in the environment variable the server of the token is read from.
*/
class CConnectionTokenStoreComp:
			public ilog::CLoggerComponentBase,
			virtual public IConnectionTokenStore,
			virtual public IServiceEnvironmentProvider
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CConnectionTokenStoreComp);
		I_REGISTER_INTERFACE(IConnectionTokenStore);
		I_REGISTER_INTERFACE(IServiceEnvironmentProvider);
		I_ASSIGN(m_filePathCompPtr, "FilePath", "File of the tokens", true, "FilePath");
		I_ASSIGN(m_secretProtectorCompPtr, "SecretProtector", "Protection of the values of the tokens", true, "SecretProtector");
	I_END_COMPONENT;

	// reimplemented (IConnectionTokenStore)
	virtual bool GetToken(const QByteArray& serviceId, const QByteArray& connectionId, TokenInfo& tokenInfo) const override;
	virtual bool StoreToken(const QByteArray& serviceId, const QByteArray& connectionId, const TokenInfo& tokenInfo, const QByteArray& value) override;
	virtual bool RemoveToken(const QByteArray& serviceId, const QByteArray& connectionId) override;

	// reimplemented (IServiceEnvironmentProvider)
	virtual Environment GetServiceEnvironment(const QByteArray& serviceId, IServiceInfo& serviceInfo) const override;

private:
	/**
		The stored tokens: services by ID, their connections by ID. Read from the file once.
	*/
	QJsonObject& GetTokens() const;
	bool Save() const;

private:
	I_REF(ifile::IFileNameParam, m_filePathCompPtr);
	I_REF(imtcrypt::ISecretProtector, m_secretProtectorCompPtr);

	mutable QMutex m_mutex;
	mutable QJsonObject m_tokens;
	mutable bool m_isLoaded = false;
};


} // namespace agentinodata


