// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

// ACF includes
#include <icomp/CComponentBase.h>
#include <ilog/TLoggerCompWrap.h>

// Agentino includes
#include <agentinodata/IProcessHost.h>


namespace agentinodata
{


/**
	QProcess-based ProcessHost: owns child handles; emits ChildExited on death.
	Supports TryAdopt(pid) for durable re-attach after agent restart.
	No image-name kill, no process-table scan.
*/
class CProcessHostComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public IProcessHost
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CProcessHostComp);
		I_REGISTER_INTERFACE(IProcessHost);
		I_ASSIGN(m_adoptPollMsAttrPtr, "AdoptPollMs", "Poll interval for adopted PID liveness", false, 1000);
	I_END_COMPONENT;

	// reimplemented (IProcessHost)
	virtual bool Spawn(const SpawnRequest& request, qint64& pid, QString& errorMessage) override;
	virtual bool TryAdopt(const AdoptRequest& request, QString& errorMessage) override;
	virtual bool SignalStop(const QByteArray& serviceId, QString& errorMessage) override;
	virtual bool ForceKill(const QByteArray& serviceId, QString& errorMessage) override;
	virtual bool IsRunning(const QByteArray& serviceId) const override;
	virtual qint64 Pid(const QByteArray& serviceId) const override;
	virtual void Release(const QByteArray& serviceId) override;

Q_SIGNALS:
	void ChildExited(const QByteArray& serviceId, int exitCode, QProcess::ExitStatus exitStatus);
	void ChildStarted(const QByteArray& serviceId, qint64 pid);
	void ChildError(const QByteArray& serviceId, QString errorMessage);

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

private Q_SLOTS:
	void OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void OnProcessError(QProcess::ProcessError error);
	void OnProcessStarted();
	void OnAdoptPoll();

private:
	struct Child
	{
		QProcess* process = nullptr;
		qint64 pid = 0;
		bool adopted = false;
		void* nativeHandle = nullptr; // Windows HANDLE for adopted, else nullptr
	};

	QByteArray ServiceIdForSender() const;
	void EnsureAdoptPollTimer();
	void ClearChildEntry(const QByteArray& serviceId, bool killSpawnedIfRunning);
	bool ChildIsAlive(const Child& child) const;
	bool TerminateOsProcess(const Child& child, bool force, QString& errorMessage) const;

	static bool IsPidAlive(qint64 pid);
	static bool QueryProcessImagePath(qint64 pid, QString& imagePath, QString& errorMessage);
	static bool ImagePathMatches(const QString& actualPath, const QString& expectedProgram);

private:
	I_ATTR(int, m_adoptPollMsAttrPtr);

	QHash<QByteArray, Child> m_children;
	QTimer* m_adoptPollTimer = nullptr;
	int m_adoptPollMs = 1000;
};


} // namespace agentinodata
