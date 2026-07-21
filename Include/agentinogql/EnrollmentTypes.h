// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QVector>


namespace agentinogql
{


enum class EnrollmentStatus
{
	Unknown,
	Pending,
	Approved,
	Suspended,
	Revoked,
	Rejected
};


inline QByteArray EnrollmentStatusToString(EnrollmentStatus status)
{
	switch (status) {
	case EnrollmentStatus::Pending: return QByteArrayLiteral("Pending");
	case EnrollmentStatus::Approved: return QByteArrayLiteral("Approved");
	case EnrollmentStatus::Suspended: return QByteArrayLiteral("Suspended");
	case EnrollmentStatus::Revoked: return QByteArrayLiteral("Revoked");
	case EnrollmentStatus::Rejected: return QByteArrayLiteral("Rejected");
	case EnrollmentStatus::Unknown:
	default: return QByteArrayLiteral("Unknown");
	}
}


inline EnrollmentStatus EnrollmentStatusFromString(const QByteArray& value)
{
	if (value == QByteArrayLiteral("Pending")) return EnrollmentStatus::Pending;
	if (value == QByteArrayLiteral("Approved")) return EnrollmentStatus::Approved;
	if (value == QByteArrayLiteral("Suspended")) return EnrollmentStatus::Suspended;
	if (value == QByteArrayLiteral("Revoked")) return EnrollmentStatus::Revoked;
	if (value == QByteArrayLiteral("Rejected")) return EnrollmentStatus::Rejected;
	return EnrollmentStatus::Unknown;
}


struct EnrollmentStatusChange
{
	EnrollmentStatus status = EnrollmentStatus::Unknown;
	QDateTime at;
	QString note;
};


struct EnrollmentRecord
{
	QByteArray agentId;
	EnrollmentStatus status = EnrollmentStatus::Unknown;
	QString claimedName;
	QString computerName;
	QString advertisedEndpoint;
	QString agentVersion;
	QString os;
	QDateTime firstSeenAt;
	QDateTime lastSeenAt;
	QDateTime approvedAt;
	QString decisionNote;
	QVector<EnrollmentStatusChange> history;
};


enum class GateDecision
{
	Active,
	Quarantine,
	Deny
};


enum class SessionStatus
{
	Connecting,
	Quarantined,
	Active,
	Suspect,
	Offline
};


} // namespace agentinogql
