// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CAgentEnrollmentRecord.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>

// ImtCore includes
#include <imtbase/imtbase.h>


namespace agentinogql
{


// public methods

CAgentEnrollmentRecord::CAgentEnrollmentRecord()
	:m_status(EnrollmentStatus::Unknown)
{
}


// reimplemented (agentinogql::IAgentEnrollmentRecord)

EnrollmentRecord CAgentEnrollmentRecord::GetRecord(const QByteArray& agentId) const
{
	EnrollmentRecord record;
	record.agentId = agentId;
	record.status = m_status;
	record.claimedName = m_claimedName;
	record.computerName = m_computerName;
	record.advertisedEndpoint = m_advertisedEndpoint;
	record.agentVersion = m_agentVersion;
	record.os = m_os;
	record.firstSeenAt = m_firstSeenAt;
	record.lastSeenAt = m_lastSeenAt;
	record.approvedAt = m_approvedAt;
	record.decisionNote = m_decisionNote;
	record.history = m_history;

	return record;
}


void CAgentEnrollmentRecord::SetRecord(const EnrollmentRecord& record)
{
	istd::CChangeNotifier changeNotifier(this);

	m_status = record.status;
	m_claimedName = record.claimedName;
	m_computerName = record.computerName;
	m_advertisedEndpoint = record.advertisedEndpoint;
	m_agentVersion = record.agentVersion;
	m_os = record.os;
	m_firstSeenAt = record.firstSeenAt;
	m_lastSeenAt = record.lastSeenAt;
	m_approvedAt = record.approvedAt;
	m_decisionNote = record.decisionNote;
	m_history = record.history;
}


// reimplemented (iser::ISerializable)

bool CAgentEnrollmentRecord::Serialize(iser::IArchive& archive)
{
	istd::CChangeNotifier notifier(archive.IsStoring() ? nullptr : this);

	bool retVal = true;

	iser::CArchiveTag statusTag("Status", "Enrollment status", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(statusTag);
	if (archive.IsStoring()){
		QByteArray statusText = EnrollmentStatusToString(m_status);
		retVal = retVal && archive.Process(statusText);
	}
	else{
		QByteArray statusText;
		retVal = retVal && archive.Process(statusText);
		m_status = EnrollmentStatusFromString(statusText);
	}
	retVal = retVal && archive.EndTag(statusTag);

	iser::CArchiveTag claimedNameTag("ClaimedName", "Claimed name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(claimedNameTag);
	retVal = retVal && archive.Process(m_claimedName);
	retVal = retVal && archive.EndTag(claimedNameTag);

	iser::CArchiveTag computerNameTag("ComputerName", "Computer name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(computerNameTag);
	retVal = retVal && archive.Process(m_computerName);
	retVal = retVal && archive.EndTag(computerNameTag);

	iser::CArchiveTag advertisedEndpointTag("AdvertisedEndpoint", "Advertised endpoint", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(advertisedEndpointTag);
	retVal = retVal && archive.Process(m_advertisedEndpoint);
	retVal = retVal && archive.EndTag(advertisedEndpointTag);

	iser::CArchiveTag agentVersionTag("AgentVersion", "Agent version", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(agentVersionTag);
	retVal = retVal && archive.Process(m_agentVersion);
	retVal = retVal && archive.EndTag(agentVersionTag);

	iser::CArchiveTag osTag("Os", "Operating system", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(osTag);
	retVal = retVal && archive.Process(m_os);
	retVal = retVal && archive.EndTag(osTag);

	iser::CArchiveTag firstSeenAtTag("FirstSeenAt", "First seen at", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(firstSeenAtTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_firstSeenAt);
	retVal = retVal && archive.EndTag(firstSeenAtTag);

	iser::CArchiveTag lastSeenAtTag("LastSeenAt", "Last seen at", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(lastSeenAtTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_lastSeenAt);
	retVal = retVal && archive.EndTag(lastSeenAtTag);

	iser::CArchiveTag approvedAtTag("ApprovedAt", "Approved at", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(approvedAtTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_approvedAt);
	retVal = retVal && archive.EndTag(approvedAtTag);

	iser::CArchiveTag decisionNoteTag("DecisionNote", "Decision note", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(decisionNoteTag);
	retVal = retVal && archive.Process(m_decisionNote);
	retVal = retVal && archive.EndTag(decisionNoteTag);

	int historyCount = imtbase::narrow_cast<int>(m_history.count());
	if (!archive.IsStoring()){
		historyCount = 0;
		m_history.clear();
	}

	iser::CArchiveTag historyListTag("History", "Status history", iser::CArchiveTag::TT_MULTIPLE);
	iser::CArchiveTag historyItemTag("Change", "Status change", iser::CArchiveTag::TT_GROUP, &historyListTag);

	retVal = retVal && archive.BeginMultiTag(historyListTag, historyItemTag, historyCount);
	for (int i = 0; i < historyCount; i++){
		retVal = retVal && archive.BeginTag(historyItemTag);

		EnrollmentStatusChange change;
		if (archive.IsStoring()){
			change = m_history[i];
		}

		iser::CArchiveTag changeStatusTag("Status", "Status", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(changeStatusTag);
		if (archive.IsStoring()){
			QByteArray statusText = EnrollmentStatusToString(change.status);
			retVal = retVal && archive.Process(statusText);
		}
		else{
			QByteArray statusText;
			retVal = retVal && archive.Process(statusText);
			change.status = EnrollmentStatusFromString(statusText);
		}
		retVal = retVal && archive.EndTag(changeStatusTag);

		iser::CArchiveTag changeAtTag("At", "At", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(changeAtTag);
		retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, change.at);
		retVal = retVal && archive.EndTag(changeAtTag);

		iser::CArchiveTag changeNoteTag("Note", "Note", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(changeNoteTag);
		retVal = retVal && archive.Process(change.note);
		retVal = retVal && archive.EndTag(changeNoteTag);

		retVal = retVal && archive.EndTag(historyItemTag);

		if (retVal && !archive.IsStoring()){
			m_history.append(change);
		}
	}
	retVal = retVal && archive.EndTag(historyListTag);

	return retVal;
}


// reimplemented (istd::IChangeable)

int CAgentEnrollmentRecord::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CAgentEnrollmentRecord::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const CAgentEnrollmentRecord* sourcePtr = dynamic_cast<const CAgentEnrollmentRecord*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_status = sourcePtr->m_status;
		m_claimedName = sourcePtr->m_claimedName;
		m_computerName = sourcePtr->m_computerName;
		m_advertisedEndpoint = sourcePtr->m_advertisedEndpoint;
		m_agentVersion = sourcePtr->m_agentVersion;
		m_os = sourcePtr->m_os;
		m_firstSeenAt = sourcePtr->m_firstSeenAt;
		m_lastSeenAt = sourcePtr->m_lastSeenAt;
		m_approvedAt = sourcePtr->m_approvedAt;
		m_decisionNote = sourcePtr->m_decisionNote;
		m_history = sourcePtr->m_history;

		return true;
	}

	return false;
}


istd::IChangeableUniquePtr CAgentEnrollmentRecord::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CAgentEnrollmentRecord);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CAgentEnrollmentRecord::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_status = EnrollmentStatus::Unknown;
	m_claimedName.clear();
	m_computerName.clear();
	m_advertisedEndpoint.clear();
	m_agentVersion.clear();
	m_os.clear();
	m_firstSeenAt = QDateTime();
	m_lastSeenAt = QDateTime();
	m_approvedAt = QDateTime();
	m_decisionNote.clear();
	m_history.clear();

	return true;
}


} // namespace agentinogql
