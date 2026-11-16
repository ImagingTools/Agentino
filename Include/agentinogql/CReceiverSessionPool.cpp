// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CReceiverSessionPool.h>


// Qt includes
#include <QtCore/QMutexLocker>
#include <QtCore/QUuid>


namespace agentinogql
{


// public methods

QByteArray CReceiverSessionPool::Add(const Session& session)
{
	const QByteArray signInId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();

	QMutexLocker locker(&m_mutex);
	m_sessions.insert(signInId, session);

	return signInId;
}


bool CReceiverSessionPool::Take(const QByteArray& signInId, Session& session)
{
	QMutexLocker locker(&m_mutex);
	auto iter = m_sessions.find(signInId);
	if (iter == m_sessions.end() || iter->expiresAt <= QDateTime::currentDateTimeUtc()){
		return false;
	}

	session = iter.value();
	m_sessions.erase(iter);

	return true;
}


QList<CReceiverSessionPool::Session> CReceiverSessionPool::TakeExpired()
{
	const QDateTime now = QDateTime::currentDateTimeUtc();
	QList<Session> expired;

	QMutexLocker locker(&m_mutex);
	for (auto iter = m_sessions.begin(); iter != m_sessions.end();){
		if (iter->expiresAt <= now){
			expired.append(iter.value());
			iter = m_sessions.erase(iter);
		}
		else{
			++iter;
		}
	}

	return expired;
}


} // namespace agentinogql


