// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{


/**
	OS port: owns one supervised child process handle per service.
	Death is reported via callback — no process-table scans.

	Adoption (Architecture residual #4): re-attach by **explicit PID** from durable
	supervisor state after agent restart. Image path is verified when provided.
	No image-name kill, no table walk.

	Derives from istd::IChangeable so the ACF model-component wrapper
	(TModelCompWrap on registration) has the change interface to override.
*/
class IProcessHost: virtual public istd::IChangeable
{
public:
	struct SpawnRequest
	{
		QByteArray serviceId;
		QString program;
		QStringList arguments;
		QString workingDirectory;
	};

	struct AdoptRequest
	{
		QByteArray serviceId;
		qint64 pid = 0;
		/** If non-empty, process image path must match (canonical or basename). */
		QString expectedProgram;
	};

	virtual bool Spawn(const SpawnRequest& request, qint64& pid, QString& errorMessage) = 0;
	/**
		Attach to an existing OS process by PID (no spawn). Used after agent restart
		when durable state still lists a live child. Fails closed on path mismatch.
	*/
	virtual bool TryAdopt(const AdoptRequest& request, QString& errorMessage) = 0;
	virtual bool SignalStop(const QByteArray& serviceId, QString& errorMessage) = 0;
	virtual bool ForceKill(const QByteArray& serviceId, QString& errorMessage) = 0;
	virtual bool IsRunning(const QByteArray& serviceId) const = 0;
	virtual qint64 Pid(const QByteArray& serviceId) const = 0;
	/**
		Drop tracking for \a serviceId. Does not kill an adopted process
		(use ForceKill/SignalStop for intentional stop). Spawned QProcess children
		that are still running are killed (agent-owned).
	*/
	virtual void Release(const QByteArray& serviceId) = 0;
};


} // namespace agentinodata
