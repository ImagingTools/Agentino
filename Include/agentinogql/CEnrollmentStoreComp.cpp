// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CEnrollmentStoreComp.h>


// Qt includes
#include <QtCore/QMutexLocker>

// ACF includes
#include <istd/CChangeNotifier.h>

// Agentino includes
#include <agentinogql/CAgentEnrollmentRecord.h>
#include <agentinogql/IAgentEnrollmentRecord.h>


namespace agentinogql
{


// public methods

bool CEnrollmentStoreComp::Approve(
			const QByteArray& agentId,
			const QString& name,
			const QString& note,
			QString& errorMessage)
{
	return SetStatus(agentId, EnrollmentStatus::Approved, note, name, errorMessage);
}


bool CEnrollmentStoreComp::Reject(
			const QByteArray& agentId,
			const QString& note,
			QString& errorMessage)
{
	return SetStatus(agentId, EnrollmentStatus::Rejected, note, QString(), errorMessage);
}


bool CEnrollmentStoreComp::Revoke(
			const QByteArray& agentId,
			const QString& note,
			QString& errorMessage)
{
	return SetStatus(agentId, EnrollmentStatus::Revoked, note, QString(), errorMessage);
}


bool CEnrollmentStoreComp::Suspend(
			const QByteArray& agentId,
			bool suspend,
			QString& errorMessage)
{
	return SetStatus(
				agentId,
				suspend ? EnrollmentStatus::Suspended : EnrollmentStatus::Approved,
				suspend ? QStringLiteral("suspended") : QStringLiteral("resumed"),
				QString(),
				errorMessage);
}


bool CEnrollmentStoreComp::ResetRejected(const QByteArray& agentId, QString& errorMessage)
{
	return SetStatus(agentId, EnrollmentStatus::Pending, QStringLiteral("reset"), QString(), errorMessage);
}


EnrollmentRecord CEnrollmentStoreComp::Get(const QByteArray& agentId) const
{
	QMutexLocker lock(&m_mutex);

	EnrollmentRecord record;
	FindRecord(agentId, record);

	return record;
}


QVector<EnrollmentRecord> CEnrollmentStoreComp::ListByStatus(EnrollmentStatus status) const
{
	QMutexLocker lock(&m_mutex);

	QVector<EnrollmentRecord> result;
	if (!m_recordCollectionCompPtr.IsValid()){
		return result;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_recordCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		EnrollmentRecord record;
		if (FindRecord(agentId, record) && record.status == status){
			result.append(record);
		}
	}

	return result;
}


QVector<EnrollmentRecord> CEnrollmentStoreComp::ListAll() const
{
	QMutexLocker lock(&m_mutex);

	QVector<EnrollmentRecord> result;
	if (!m_recordCollectionCompPtr.IsValid()){
		return result;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_recordCollectionCompPtr->GetElementIds();
	result.reserve(agentIds.count());
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		EnrollmentRecord record;
		if (FindRecord(agentId, record)){
			result.append(record);
		}
	}

	return result;
}


GateDecision CEnrollmentStoreComp::Admit(
			const AgentIdentity& identity,
			const AgentAttributes& attributes,
			EnrollmentRecord& recordOut)
{
	QMutexLocker lock(&m_mutex);

	const QByteArray agentId = identity.agentId;
	if (agentId.isEmpty()){
		return GateDecision::Deny;
	}

	const QDateTime now = QDateTime::currentDateTimeUtc();

	EnrollmentRecord record;
	if (!FindRecord(agentId, record)){
		record.agentId = agentId;
		// A first-seen agent is ALWAYS Pending — never auto-approved. It is approved
		// explicitly from the Agents list, so there is no bootstrap hole.
		record.status = EnrollmentStatus::Pending;
		record.claimedName = attributes.claimedName;
		record.computerName = attributes.computerName;
		record.advertisedEndpoint = attributes.advertisedEndpoint;
		record.agentVersion = attributes.agentVersion;
		record.os = attributes.os;
		record.firstSeenAt = now;
		record.lastSeenAt = now;
		AppendHistory(record, EnrollmentStatus::Pending, QStringLiteral("first connect"));

		if (!StoreRecord(record)){
			return GateDecision::Deny;
		}

		recordOut = record;
		istd::CChangeNotifier notifier(this);
		Q_UNUSED(notifier);

		return GateDecision::Quarantine;
	}

	record.lastSeenAt = now;
	record.computerName = attributes.computerName.isEmpty() ? record.computerName : attributes.computerName;
	record.advertisedEndpoint = attributes.advertisedEndpoint.isEmpty() ? record.advertisedEndpoint : attributes.advertisedEndpoint;
	record.agentVersion = attributes.agentVersion.isEmpty() ? record.agentVersion : attributes.agentVersion;
	record.os = attributes.os.isEmpty() ? record.os : attributes.os;
	StoreRecord(record);
	recordOut = record;

	switch (record.status) {
	case EnrollmentStatus::Approved:
		return GateDecision::Active;
	case EnrollmentStatus::Pending:
	case EnrollmentStatus::Unknown:
		return GateDecision::Quarantine;
	case EnrollmentStatus::Suspended:
		return GateDecision::Quarantine;
	case EnrollmentStatus::Rejected:
	case EnrollmentStatus::Revoked:
	default:
		return GateDecision::Deny;
	}
}


// private methods

bool CEnrollmentStoreComp::SetStatus(
			const QByteArray& agentId,
			EnrollmentStatus status,
			const QString& note,
			const QString& name,
			QString& errorMessage)
{
	QMutexLocker lock(&m_mutex);

	EnrollmentRecord record;
	if (!FindRecord(agentId, record)){
		errorMessage = QStringLiteral("Agent enrollment record not found");

		return false;
	}

	record.status = status;
	if (!name.isEmpty()){
		record.claimedName = name;
	}
	record.decisionNote = note;
	if (status == EnrollmentStatus::Approved){
		record.approvedAt = QDateTime::currentDateTimeUtc();
	}
	AppendHistory(record, status, note);

	if (!StoreRecord(record)){
		errorMessage = QStringLiteral("Failed to persist enrollment store");

		return false;
	}

	istd::CChangeNotifier notifier(this);
	Q_UNUSED(notifier);

	return true;
}


void CEnrollmentStoreComp::AppendHistory(
			EnrollmentRecord& record,
			EnrollmentStatus status,
			const QString& note)
{
	EnrollmentStatusChange change;
	change.status = status;
	change.at = QDateTime::currentDateTimeUtc();
	change.note = note;
	record.history.append(change);
}


bool CEnrollmentStoreComp::FindRecord(const QByteArray& agentId, EnrollmentRecord& record) const
{
	if (!m_recordCollectionCompPtr.IsValid()){
		return false;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_recordCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		return false;
	}

	const IAgentEnrollmentRecord* recordPtr = dynamic_cast<const IAgentEnrollmentRecord*>(dataPtr.GetPtr());
	if (recordPtr == nullptr){
		return false;
	}

	record = recordPtr->GetRecord(agentId);

	return true;
}


bool CEnrollmentStoreComp::StoreRecord(const EnrollmentRecord& record)
{
	if (!m_recordCollectionCompPtr.IsValid() || record.agentId.isEmpty()){
		return false;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_recordCollectionCompPtr->GetObjectData(record.agentId, dataPtr)){
		IAgentEnrollmentRecord* recordPtr = dynamic_cast<IAgentEnrollmentRecord*>(dataPtr.GetPtr());
		if (recordPtr == nullptr){
			return false;
		}

		recordPtr->SetRecord(record);

		return m_recordCollectionCompPtr->SetObjectData(record.agentId, *dataPtr.GetPtr());
	}

	CAgentEnrollmentRecord newRecord;
	newRecord.SetRecord(record);

	const QByteArray insertedId = m_recordCollectionCompPtr->InsertNewObject(
				"AgentEnrollmentRecord",
				QString::fromUtf8(record.agentId),
				QString(),
				&newRecord,
				record.agentId);

	return !insertedId.isEmpty();
}


} // namespace agentinogql
