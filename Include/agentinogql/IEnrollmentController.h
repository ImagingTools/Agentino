// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

// ACF includes
#include <istd/IChangeable.h>
#include <istd/IPolymorphic.h>

// Agentino includes
#include <agentinogql/EnrollmentTypes.h>


namespace agentinogql
{


/**
	Sole writer of EnrollmentRecord (Architecture Audit §4.13 / §7.7).
*/
class IEnrollmentController: virtual public istd::IChangeable
{
public:
	virtual bool Approve(
				const QByteArray& agentId,
				const QString& name,
				const QString& note,
				QString& errorMessage) = 0;
	virtual bool Reject(
				const QByteArray& agentId,
				const QString& note,
				QString& errorMessage) = 0;
	virtual bool Revoke(
				const QByteArray& agentId,
				const QString& note,
				QString& errorMessage) = 0;
	virtual bool Suspend(
				const QByteArray& agentId,
				bool suspend,
				QString& errorMessage) = 0;
	virtual bool ResetRejected(
				const QByteArray& agentId,
				QString& errorMessage) = 0;

	virtual EnrollmentRecord Get(const QByteArray& agentId) const = 0;
	virtual QVector<EnrollmentRecord> ListByStatus(EnrollmentStatus status) const = 0;
	virtual QVector<EnrollmentRecord> ListAll() const = 0;
};


/**
	Admission gate for connecting agents.

	There is deliberately **no cryptographic proof of identity**: an agent is
	recognised by the agent id it reports. The gate's job is to make a first-seen
	agent land in Pending and stay inert until an operator approves it — not to
	authenticate it. Adding key-based identity later means extending AgentIdentity
	and verifying it inside Admit; nothing else needs to change.
*/
class IEnrollmentGate: virtual public istd::IPolymorphic
{
public:
	struct AgentIdentity
	{
		QByteArray agentId;
	};

	struct AgentAttributes
	{
		QString computerName;
		QString agentVersion;
		QString os;
		QString advertisedEndpoint;
		QString claimedName;
	};

	virtual GateDecision Admit(
				const AgentIdentity& identity,
				const AgentAttributes& attributes,
				EnrollmentRecord& recordOut) = 0;
};


} // namespace agentinogql
