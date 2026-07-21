// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>


namespace agentinodata
{


/**
	Explicit service FSM states (Architecture Audit §4.6).
	Replaces counter-driven SS_* transitions over time.
*/
enum class ServiceRuntimeStatus
{
	Stopped,
	Starting,
	Running,
	Stopping,
	Crashed,
	Failed
};


enum class ServiceFailureReason
{
	None,
	StartFailed,
	StopTimeout,
	CrashLooping,
	SpawnError,
	Unknown
};


struct RestartPolicy
{
	int maxRestarts = 3;
	/** Crash-restart budget window (sliding). */
	int windowSeconds = 60;
	int backoffMs = 1000;
	/**
		Seconds of continuous Running before restartCount is cleared.
		Must not reset on bare ChildStarted — start→immediate-crash would never hit CrashLooping.
	*/
	int stableSeconds = 15;
};


struct ServiceRuntimeState
{
	QByteArray serviceId;
	ServiceRuntimeStatus status = ServiceRuntimeStatus::Stopped;
	ServiceFailureReason reason = ServiceFailureReason::None;
	qint64 pid = 0;
	QDateTime observedAt;
	/** Restarts counted inside the current RestartPolicy window. */
	int restartCount = 0;
	/** Start of the current crash-restart window (UTC). */
	QDateTime restartWindowStart;
};


inline QByteArray ServiceRuntimeStatusToString(ServiceRuntimeStatus status)
{
	switch (status) {
	case ServiceRuntimeStatus::Starting: return QByteArrayLiteral("Starting");
	case ServiceRuntimeStatus::Running: return QByteArrayLiteral("Running");
	case ServiceRuntimeStatus::Stopping: return QByteArrayLiteral("Stopping");
	case ServiceRuntimeStatus::Crashed: return QByteArrayLiteral("Crashed");
	case ServiceRuntimeStatus::Failed: return QByteArrayLiteral("Failed");
	case ServiceRuntimeStatus::Stopped:
	default: return QByteArrayLiteral("Stopped");
	}
}


inline QByteArray ServiceFailureReasonToString(ServiceFailureReason reason)
{
	switch (reason) {
	case ServiceFailureReason::StartFailed: return QByteArrayLiteral("StartFailed");
	case ServiceFailureReason::StopTimeout: return QByteArrayLiteral("StopTimeout");
	case ServiceFailureReason::CrashLooping: return QByteArrayLiteral("CrashLooping");
	case ServiceFailureReason::SpawnError: return QByteArrayLiteral("SpawnError");
	case ServiceFailureReason::Unknown: return QByteArrayLiteral("Unknown");
	case ServiceFailureReason::None:
	default: return QByteArrayLiteral("None");
	}
}


} // namespace agentinodata
