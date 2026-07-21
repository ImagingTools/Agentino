// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinogql/IAgentEnrollmentRecord.h>


namespace agentinogql
{


class CAgentEnrollmentRecord: virtual public IAgentEnrollmentRecord
{
public:
	CAgentEnrollmentRecord();

	// reimplemented (agentinogql::IAgentEnrollmentRecord)
	virtual EnrollmentRecord GetRecord(const QByteArray& agentId) const override;
	virtual void SetRecord(const EnrollmentRecord& record) override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

private:
	// agentId excluded on purpose - see IAgentEnrollmentRecord.
	EnrollmentStatus m_status;
	QString m_claimedName;
	QString m_computerName;
	QString m_advertisedEndpoint;
	QString m_agentVersion;
	QString m_os;
	QDateTime m_firstSeenAt;
	QDateTime m_lastSeenAt;
	QDateTime m_approvedAt;
	QString m_decisionNote;
	QVector<EnrollmentStatusChange> m_history;
};


} // namespace agentinogql
