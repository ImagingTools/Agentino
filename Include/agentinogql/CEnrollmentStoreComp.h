// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMutex>

// ACF includes
#include <icomp/CComponentBase.h>
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinogql/IEnrollmentController.h>


namespace agentinogql
{


/**
	AgentEnrollment store + EnrollmentController + EnrollmentGate.

	Records are items of RecordCollection (agent id is the collection's own element id,
	see IAgentEnrollmentRecord), durably persisted by a FileAutoPersistence wired to that
	collection in the ACC (see EnrollmentRecordCollection in ServerBase.acc) - this
	component does no file I/O of its own. A first-seen agent is always stored as Pending
	and admitted only as Quarantine until an operator approves it; there is no
	cryptographic identity check (see IEnrollmentGate).
*/
class CEnrollmentStoreComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public IEnrollmentController,
			virtual public IEnrollmentGate
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CEnrollmentStoreComp);
		I_REGISTER_INTERFACE(IEnrollmentController);
		I_REGISTER_INTERFACE(IEnrollmentGate);
		I_ASSIGN(m_recordCollectionCompPtr, "RecordCollection", "Durable per-agent enrollment records (agent id = element id)", true, "RecordCollection");
	I_END_COMPONENT;

	// reimplemented (IEnrollmentController)
	virtual bool Approve(const QByteArray& agentId, const QString& name, const QString& note, QString& errorMessage) override;
	virtual bool Reject(const QByteArray& agentId, const QString& note, QString& errorMessage) override;
	virtual bool Revoke(const QByteArray& agentId, const QString& note, QString& errorMessage) override;
	virtual bool Suspend(const QByteArray& agentId, bool suspend, QString& errorMessage) override;
	virtual bool ResetRejected(const QByteArray& agentId, QString& errorMessage) override;
	virtual EnrollmentRecord Get(const QByteArray& agentId) const override;
	virtual QVector<EnrollmentRecord> ListByStatus(EnrollmentStatus status) const override;
	virtual QVector<EnrollmentRecord> ListAll() const override;

	// reimplemented (IEnrollmentGate)
	virtual GateDecision Admit(
				const AgentIdentity& identity,
				const AgentAttributes& attributes,
				EnrollmentRecord& recordOut) override;

private:
	bool SetStatus(
				const QByteArray& agentId,
				EnrollmentStatus status,
				const QString& note,
				const QString& name,
				QString& errorMessage);
	void AppendHistory(EnrollmentRecord& record, EnrollmentStatus status, const QString& note);

	/** \return false when \a agentId has no record. */
	bool FindRecord(const QByteArray& agentId, EnrollmentRecord& record) const;
	/** Updates the existing record for record.agentId, or inserts a new one. */
	bool StoreRecord(const EnrollmentRecord& record);

private:
	I_REF(imtbase::IObjectCollection, m_recordCollectionCompPtr);

	mutable QMutex m_mutex;
};


} // namespace agentinogql
