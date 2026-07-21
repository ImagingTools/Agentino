// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QTimer>

// ACF includes
#include <icomp/CComponentBase.h>
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceFsm.h>
#include <agentinodata/IProcessHost.h>
#include <agentinodata/IServiceController.h>
#include <agentinodata/IServiceSupervisor.h>
#include <agentinodata/IServiceTypeCatalog.h>


namespace agentinodata
{


/**
	Event-sourced service supervisor: owns FSM + child handles via ProcessHost;
	is the sole writer of runtime status; implements IServiceController for GQL.

	Durable PID state enables TryAdopt after agent hard crash (Architecture #4)
	without process-table scans.
*/
class CServiceSupervisorComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public IServiceSupervisor,
			virtual public IServiceController
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceSupervisorComp);
		I_REGISTER_INTERFACE(IServiceSupervisor);
		I_REGISTER_INTERFACE(IServiceController);
		I_REGISTER_INTERFACE(IServiceStatusProvider);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Authoritative service descriptors", true, "ServiceCollection");
		I_ASSIGN(m_processHostCompPtr, "ProcessHost", "OS child-process port", true, "ProcessHost");
		I_ASSIGN(m_typeCatalogCompPtr, "ServiceTypeCatalog", "Plugin catalog (once per type)", false, "ServiceTypeCatalog");
		I_ASSIGN(m_stopTimeoutMsAttrPtr, "StopTimeoutMs", "Graceful stop timeout before force-kill", false, 10000);
		I_ASSIGN(m_stableSecondsAttrPtr, "StableSeconds", "Running duration before clearing restart budget", false, 15);
		I_ASSIGN(m_runtimeStatePathAttrPtr, "RuntimeStatePath", "JSON file for durable service PIDs (adoption)", false, "ServiceRuntimePids.json");
	I_END_COMPONENT;

	// reimplemented (IServiceSupervisor)
	virtual bool Start(const QByteArray& serviceId, QString& errorMessage) override;
	virtual bool Stop(const QByteArray& serviceId, QString& errorMessage) override;
	virtual ServiceRuntimeState GetState(const QByteArray& serviceId) const override;

	// reimplemented (IServiceController / IServiceStatusProvider)
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override;
	virtual bool StartService(const QByteArray& serviceId) override;
	virtual bool StopService(const QByteArray& serviceId) override;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

private Q_SLOTS:
	void OnChildExited(const QByteArray& serviceId, int exitCode, QProcess::ExitStatus exitStatus);
	void OnChildStarted(const QByteArray& serviceId, qint64 pid);
	void OnChildError(const QByteArray& serviceId, QString errorMessage);
	void OnStopTimeout();
	void OnRestartBackoff();
	void OnHealthStable();

private:
	struct DurablePidEntry
	{
		qint64 pid = 0;
		/** Program/script actually started (SpawnRequest.program). */
		QString program;
		/** Service executable path — preferred for OS image-path verify when script differs. */
		QString executable;
	};

	bool ApplyEvent(
				const QByteArray& serviceId,
				CServiceFsm::Event event,
				ServiceFailureReason forcedReason = ServiceFailureReason::None);
	void EmitStatus(const ServiceRuntimeState& state);
	ServiceRuntimeState& EnsureState(const QByteArray& serviceId);
	bool BuildSpawnRequest(const QByteArray& serviceId, IProcessHost::SpawnRequest& request, QString& errorMessage) const;
	RestartPolicy PolicyFor(const QByteArray& serviceId) const;
	void RefreshTypeCatalog(const QByteArray& serviceId) const;
	static IServiceStatusInfo::ServiceStatus ToLegacyStatus(ServiceRuntimeStatus status);
	void AutoStartServices();
	void CancelTimer(QHash<QByteArray, QTimer*>& timers, const QByteArray& serviceId);
	void ArmHealthTimer(const QByteArray& serviceId);
	bool SpawnChild(const QByteArray& serviceId, QString& errorMessage);
	/** Align FSM to Running when host already tracks a live child. */
	bool AlignRunning(const QByteArray& serviceId, QString& errorMessage);
	/**
		Try re-attach from durable PID state. On success host tracks the process
		and FSM is aligned to Running. On failure clears the stale durable entry.
	*/
	bool TryAdoptFromDurableState(const QByteArray& serviceId, QString& errorMessage);
	void PersistPid(const QByteArray& serviceId, qint64 pid, const QString& program, const QString& executable);
	void ClearPersistedPid(const QByteArray& serviceId);
	void LoadDurableState();
	void SaveDurableState() const;
	QString ResolveRuntimeStatePath() const;

private:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(IProcessHost, m_processHostCompPtr);
	I_REF(IServiceTypeCatalog, m_typeCatalogCompPtr);
	I_ATTR(int, m_stopTimeoutMsAttrPtr);
	I_ATTR(int, m_stableSecondsAttrPtr);
	I_ATTR(QString, m_runtimeStatePathAttrPtr);

	QHash<QByteArray, ServiceRuntimeState> m_states;
	QHash<QByteArray, QTimer*> m_stopTimers;
	QHash<QByteArray, QTimer*> m_restartTimers;
	QHash<QByteArray, QTimer*> m_healthTimers;
	QHash<QByteArray, DurablePidEntry> m_durablePids;
	int m_stopTimeoutMs = 10000;
	int m_stableSeconds = 15;
	QString m_runtimeStatePath;
};


} // namespace agentinodata
