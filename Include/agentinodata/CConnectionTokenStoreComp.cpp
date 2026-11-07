// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CConnectionTokenStoreComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSaveFile>

// ImtCore includes
#include <imtclientgql/IServerCredentialProvider.h>


namespace agentinodata
{


namespace
{


const QString s_tokenIdKey = QStringLiteral("tokenId");
const QString s_receiverKey = QStringLiteral("receiverProductId");
const QString s_scopesKey = QStringLiteral("scopes");
const QString s_issuedAtKey = QStringLiteral("issuedAt");
const QString s_valueKey = QStringLiteral("value");


IConnectionTokenStore::TokenInfo ReadTokenInfo(const QJsonObject& tokenObject)
{
	IConnectionTokenStore::TokenInfo tokenInfo;
	tokenInfo.tokenId = tokenObject.value(s_tokenIdKey).toString().toUtf8();
	tokenInfo.receiverProductId = tokenObject.value(s_receiverKey).toString().toUtf8();
	for (const QJsonValue& scope : tokenObject.value(s_scopesKey).toArray()){
		tokenInfo.scopes.append(scope.toString().toUtf8());
	}
	tokenInfo.issuedAt = QDateTime::fromString(tokenObject.value(s_issuedAtKey).toString(), Qt::ISODate);

	return tokenInfo;
}


} // anonymous namespace


// public methods

// reimplemented (IConnectionTokenStore)

bool CConnectionTokenStoreComp::GetToken(const QByteArray& serviceId, const QByteArray& connectionId, TokenInfo& tokenInfo) const
{
	QMutexLocker locker(&m_mutex);

	const QJsonObject tokenObject = GetTokens().value(QString::fromUtf8(serviceId)).toObject().value(QString::fromUtf8(connectionId)).toObject();
	if (tokenObject.isEmpty()){
		return false;
	}

	tokenInfo = ReadTokenInfo(tokenObject);

	return true;
}


bool CConnectionTokenStoreComp::StoreToken(const QByteArray& serviceId, const QByteArray& connectionId, const TokenInfo& tokenInfo, const QByteArray& value)
{
	QMutexLocker locker(&m_mutex);

	QJsonObject& tokens = GetTokens();
	QJsonObject serviceObject = tokens.value(QString::fromUtf8(serviceId)).toObject();
	QJsonObject tokenObject = serviceObject.value(QString::fromUtf8(connectionId)).toObject();

	if (!value.isEmpty()){
		QByteArray protectedValue;
		if (!m_secretProtectorCompPtr->Protect(value, protectedValue)){
			SendErrorMessage(0, QStringLiteral("Unable to protect the token of connection '%1'").arg(QString::fromUtf8(connectionId)), "CConnectionTokenStoreComp");

			return false;
		}

		tokenObject[s_valueKey] = QString::fromLatin1(protectedValue.toBase64());
	}
	else if (!tokenObject.contains(s_valueKey)){
		return false;
	}

	QJsonArray scopes;
	for (const QByteArray& scope : tokenInfo.scopes){
		scopes.append(QString::fromUtf8(scope));
	}

	tokenObject[s_tokenIdKey] = QString::fromUtf8(tokenInfo.tokenId);
	tokenObject[s_receiverKey] = QString::fromUtf8(tokenInfo.receiverProductId);
	tokenObject[s_scopesKey] = scopes;
	tokenObject[s_issuedAtKey] = tokenInfo.issuedAt.toUTC().toString(Qt::ISODate);

	serviceObject[QString::fromUtf8(connectionId)] = tokenObject;
	tokens[QString::fromUtf8(serviceId)] = serviceObject;

	return Save();
}


bool CConnectionTokenStoreComp::RemoveToken(const QByteArray& serviceId, const QByteArray& connectionId)
{
	QMutexLocker locker(&m_mutex);

	QJsonObject& tokens = GetTokens();
	QJsonObject serviceObject = tokens.value(QString::fromUtf8(serviceId)).toObject();
	if (!serviceObject.contains(QString::fromUtf8(connectionId))){
		return true;
	}

	serviceObject.remove(QString::fromUtf8(connectionId));
	if (serviceObject.isEmpty()){
		tokens.remove(QString::fromUtf8(serviceId));
	}
	else{
		tokens[QString::fromUtf8(serviceId)] = serviceObject;
	}

	return Save();
}


// reimplemented (IServiceEnvironmentProvider)

IServiceEnvironmentProvider::Environment CConnectionTokenStoreComp::GetServiceEnvironment(const QByteArray& serviceId) const
{
	QMutexLocker locker(&m_mutex);

	Environment environment;

	const QJsonObject serviceObject = GetTokens().value(QString::fromUtf8(serviceId)).toObject();
	for (auto it = serviceObject.constBegin(); it != serviceObject.constEnd(); ++it){
		const QJsonObject tokenObject = it.value().toObject();

		QByteArray value;
		if (!m_secretProtectorCompPtr->Unprotect(QByteArray::fromBase64(tokenObject.value(s_valueKey).toString().toLatin1()), value)){
			SendErrorMessage(0, QStringLiteral("Unable to read the token of connection '%1' of service '%2'").arg(it.key(), QString::fromUtf8(serviceId)), "CConnectionTokenStoreComp");

			continue;
		}

		const QByteArray receiverProductId = tokenObject.value(s_receiverKey).toString().toUtf8();
		environment.insert(QString::fromUtf8(imtclientgql::GetServerCredentialVariableName(receiverProductId)), QString::fromUtf8(value));
	}

	return environment;
}


// private methods

QJsonObject& CConnectionTokenStoreComp::GetTokens() const
{
	if (!m_isLoaded){
		m_isLoaded = true;

		QFile file(m_filePathCompPtr->GetPath());
		if (file.open(QIODevice::ReadOnly)){
			m_tokens = QJsonDocument::fromJson(file.readAll()).object();
		}
	}

	return m_tokens;
}


bool CConnectionTokenStoreComp::Save() const
{
	const QString path = m_filePathCompPtr->GetPath();
	QDir().mkpath(QFileInfo(path).absolutePath());

	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)){
		SendErrorMessage(0, QStringLiteral("Unable to write the tokens to '%1'").arg(path), "CConnectionTokenStoreComp");

		return false;
	}

	file.write(QJsonDocument(m_tokens).toJson(QJsonDocument::Compact));
	if (!file.commit()){
		SendErrorMessage(0, QStringLiteral("Unable to write the tokens to '%1'").arg(path), "CConnectionTokenStoreComp");

		return false;
	}

	// Without a protection service of the system the file is what keeps the values
	QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

	return true;
}


} // namespace agentinodata


