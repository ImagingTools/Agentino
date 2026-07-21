// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceSupervisorComp.h>


// Qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QThread>

// ACF includes
#include <istd/CChangeNotifier.h>

// Agentino includes
#include <agentinodata/CProcessHostComp.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusProvider.h>


namespace agentinodata
{


// public methods

bool CServiceSupervisorComp::Start(const QByteArray& serviceId, QString& errorMessage)
{
	if (!m_processHostCompPtr.IsValid() || !m_serviceCollectionCompPtr.IsValid()) {
		errorMessage = QStringLiteral("Supervisor dependencies not configured");
		return false;
	}

	RefreshTypeCatalog(serviceId);
	CancelTimer(m_restartTimers, serviceId);

	// Host already tracks a live child — align FSM, do not double-spawn.
	if (m_processHostCompPtr->IsRunning(serviceId)) {
		return AlignRunning(serviceId, errorMessage);
	}

	// After agent hard restart: re-attach by durable PID before spawning a duplicate.
	if (TryAdoptFromDurableState(serviceId, errorMessage)) {
		errorMessage.clear();
		return AlignRunning(serviceId, errorMessage);
	}
	errorMessage.clear();

	ServiceRuntimeState& state = EnsureState(serviceId);

	// Stuck Starting/Running without a process: recover by spawning.
	// Fresh Start / crash-restart: transition into Starting via FSM first.
	if (state.status == ServiceRuntimeStatus::Starting) {
		// Already Starting (or stuck) — fall through to SpawnChild.
	}
	else if (state.status == ServiceRuntimeStatus::Running) {
		// Desync: marked Running but host has no child.
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);
		if (!ApplyEvent(serviceId, CServiceFsm::Event::Start)) {
			errorMessage = QStringLiteral("Invalid Start transition after desync from Running");
			return false;
		}
	}
	else if (!ApplyEvent(serviceId, CServiceFsm::Event::Start)) {
		// Stopped / Crashed / Failed → Starting (incl. post-backoff crash restart).
		errorMessage = QStringLiteral("Invalid Start transition from %1")
					.arg(QString::fromUtf8(ServiceRuntimeStatusToString(state.status)));
		return false;
	}

	return SpawnChild(serviceId, errorMessage);
}


bool CServiceSupervisorComp::Stop(const QByteArray& serviceId, QString& errorMessage)
{
	if (!m_processHostCompPtr.IsValid()) {
		errorMessage = QStringLiteral("ProcessHost not configured");
		return false;
	}

	CancelTimer(m_restartTimers, serviceId);
	CancelTimer(m_healthTimers, serviceId);

	ServiceRuntimeState& state = EnsureState(serviceId);
	if (state.status == ServiceRuntimeStatus::Stopped
				|| state.status == ServiceRuntimeStatus::Failed) {
		if (!m_processHostCompPtr->IsRunning(serviceId)) {
			return true;
		}
	}

	if (!ApplyEvent(serviceId, CServiceFsm::Event::Stop)) {
		// Unexpected FSM state (e.g. CrashLooping Failed) — still request OS stop if child lives.
		errorMessage = QStringLiteral("Invalid Stop transition from %1")
					.arg(QString::fromUtf8(ServiceRuntimeStatusToString(state.status)));
		if (!m_processHostCompPtr->IsRunning(serviceId)) {
			return false;
		}
	}

	if (!m_processHostCompPtr->SignalStop(serviceId, errorMessage)) {
		// No supervised child — only mark stopped if host agrees the process is gone.
		if (!m_processHostCompPtr->IsRunning(serviceId)) {
			if (EnsureState(serviceId).status == ServiceRuntimeStatus::Stopping) {
				ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);
			}
			return true;
		}
		return false;
	}

	// SignalStop may block in waitForFinished and deliver ChildExited before we return
	// (same-thread QProcess signals). Do not arm the stop timer if already stopped.
	if (!m_processHostCompPtr->IsRunning(serviceId)) {
		if (EnsureState(serviceId).status == ServiceRuntimeStatus::Stopping) {
			ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);
		}
		CancelTimer(m_stopTimers, serviceId);
		return true;
	}

	QTimer* timer = m_stopTimers.value(serviceId, nullptr);
	if (timer == nullptr) {
		timer = new QTimer(this);
		timer->setSingleShot(true);
		timer->setProperty("serviceId", serviceId);
		connect(timer, &QTimer::timeout, this, &CServiceSupervisorComp::OnStopTimeout);
		m_stopTimers.insert(serviceId, timer);
	}
	timer->start(m_stopTimeoutMs);
	return true;
}


ServiceRuntimeState CServiceSupervisorComp::GetState(const QByteArray& serviceId) const
{
	return m_states.value(serviceId, ServiceRuntimeState{serviceId});
}


IServiceStatusInfo::ServiceStatus CServiceSupervisorComp::GetServiceStatus(const QByteArray& serviceId) const
{
	return ToLegacyStatus(GetState(serviceId).status);
}


bool CServiceSupervisorComp::StartService(const QByteArray& serviceId)
{
	// GQL StartService is handled on imtrest::CWorkerThread. ProcessHost / QTimer children
	// of this supervisor live on the main thread — never touch them from the worker.
	if (QThread::currentThread() != thread()) {
		bool ok = false;
		const bool invoked = QMetaObject::invokeMethod(
					this,
					[this, serviceId, &ok]() {
						ok = StartService(serviceId);
					},
					Qt::BlockingQueuedConnection);
		return invoked && ok;
	}

	QString error;
	return Start(serviceId, error);
}


bool CServiceSupervisorComp::StopService(const QByteArray& serviceId)
{
	if (QThread::currentThread() != thread()) {
		bool ok = false;
		const bool invoked = QMetaObject::invokeMethod(
					this,
					[this, serviceId, &ok]() {
						ok = StopService(serviceId);
					},
					Qt::BlockingQueuedConnection);
		return invoked && ok;
	}

	QString error;
	return Stop(serviceId, error);
}


// protected methods

void CServiceSupervisorComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();
	if (m_stopTimeoutMsAttrPtr.IsValid() && *m_stopTimeoutMsAttrPtr > 0) {
		m_stopTimeoutMs = *m_stopTimeoutMsAttrPtr;
	}
	if (m_stableSecondsAttrPtr.IsValid() && *m_stableSecondsAttrPtr > 0) {
		m_stableSeconds = *m_stableSecondsAttrPtr;
	}
	m_runtimeStatePath = ResolveRuntimeStatePath();
	LoadDurableState();

	CProcessHostComp* host = dynamic_cast<CProcessHostComp*>(m_processHostCompPtr.GetPtr());
	if (host != nullptr) {
		connect(host, &CProcessHostComp::ChildExited, this, &CServiceSupervisorComp::OnChildExited);
		connect(host, &CProcessHostComp::ChildStarted, this, &CServiceSupervisorComp::OnChildStarted);
		connect(host, &CProcessHostComp::ChildError, this, &CServiceSupervisorComp::OnChildError);
	}

	AutoStartServices();
}


void CServiceSupervisorComp::OnComponentDestroyed()
{
	// Stop running children on shutdown
	const QList<QByteArray> ids = m_states.keys();
	for (const QByteArray& id : ids) {
		const ServiceRuntimeStatus st = m_states[id].status;
		if (st == ServiceRuntimeStatus::Running || st == ServiceRuntimeStatus::Starting) {
			QString err;
			Stop(id, err);
		}
	}

	qDeleteAll(m_stopTimers);
	m_stopTimers.clear();
	qDeleteAll(m_restartTimers);
	m_restartTimers.clear();
	qDeleteAll(m_healthTimers);
	m_healthTimers.clear();
	BaseClass::OnComponentDestroyed();
}


// private slots

void CServiceSupervisorComp::OnChildExited(
			const QByteArray& serviceId,
			int exitCode,
			QProcess::ExitStatus exitStatus)
{
	Q_UNUSED(exitCode);
	Q_UNUSED(exitStatus);

	CancelTimer(m_stopTimers, serviceId);
	CancelTimer(m_healthTimers, serviceId);
	// Process is gone — drop durable PID so a later agent restart does not adopt a dead id.
	ClearPersistedPid(serviceId);

	ServiceRuntimeState& state = EnsureState(serviceId);
	const ServiceRuntimeStatus previous = state.status;

	if (previous == ServiceRuntimeStatus::Stopping) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);
		return;
	}

	if (previous == ServiceRuntimeStatus::Starting) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited, ServiceFailureReason::StartFailed);
		return;
	}

	if (previous == ServiceRuntimeStatus::Running) {
		// Running → Crashed. Stay Crashed for the backoff; do NOT enter Starting yet
		// (that would make Start() early-return without spawning).
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);

		imtbase::IObjectCollection::DataPtr dataPtr;
		bool autoStart = false;
		if (m_serviceCollectionCompPtr.IsValid()
					&& m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)) {
			IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
			if (info != nullptr) {
				autoStart = info->IsAutoStart();
			}
		}

		if (!autoStart) {
			return;
		}

		const RestartPolicy policy = PolicyFor(serviceId);
		state = EnsureState(serviceId);
		const QDateTime now = QDateTime::currentDateTimeUtc();
		if (!state.restartWindowStart.isValid()
					|| state.restartWindowStart.secsTo(now) >= policy.windowSeconds) {
			state.restartCount = 0;
			state.restartWindowStart = now;
		}
		if (state.restartCount < policy.maxRestarts) {
			++state.restartCount;
			// Remain Crashed until backoff fires → Start() does Crashed+Start → Starting + spawn.
			QTimer* timer = m_restartTimers.value(serviceId, nullptr);
			if (timer == nullptr) {
				timer = new QTimer(this);
				timer->setSingleShot(true);
				timer->setProperty("serviceId", serviceId);
				connect(timer, &QTimer::timeout, this, &CServiceSupervisorComp::OnRestartBackoff);
				m_restartTimers.insert(serviceId, timer);
			}
			timer->start(policy.backoffMs * qMax(1, state.restartCount));
		}
		else {
			ApplyEvent(serviceId, CServiceFsm::Event::RestartBudgetExhausted, ServiceFailureReason::CrashLooping);
		}
	}
}


void CServiceSupervisorComp::OnChildStarted(const QByteArray& serviceId, qint64 pid)
{
	ServiceRuntimeState& state = EnsureState(serviceId);
	state.pid = pid;
	ApplyEvent(serviceId, CServiceFsm::Event::ChildReady);
	// Budget is cleared only after stable Running (health timer), not on bare start.
	ArmHealthTimer(serviceId);

	IProcessHost::SpawnRequest request;
	QString unused;
	QString program;
	QString executable;
	if (BuildSpawnRequest(serviceId, request, unused)) {
		program = request.program;
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceCollectionCompPtr.IsValid()
				&& m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)) {
		IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
		if (info != nullptr) {
			executable = QString::fromUtf8(info->GetServicePath());
		}
	}
	PersistPid(serviceId, pid, program, executable);
}


void CServiceSupervisorComp::OnChildError(const QByteArray& serviceId, QString errorMessage)
{
	Q_UNUSED(errorMessage);
	ServiceRuntimeState& state = EnsureState(serviceId);
	if (state.status == ServiceRuntimeStatus::Starting) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited, ServiceFailureReason::SpawnError);
	}
}


void CServiceSupervisorComp::OnStopTimeout()
{
	QTimer* timer = qobject_cast<QTimer*>(sender());
	if (timer == nullptr) {
		return;
	}
	const QByteArray serviceId = timer->property("serviceId").toByteArray();
	QString error;
	if (m_processHostCompPtr.IsValid()) {
		m_processHostCompPtr->ForceKill(serviceId, error);
	}

	ServiceRuntimeState& state = EnsureState(serviceId);
	if (state.status != ServiceRuntimeStatus::Stopping) {
		// ChildExited may already have moved Stopping → Stopped while kill ran.
		return;
	}

	// Prefer Stopped when the OS process is actually gone (normal after kill).
	// KillTimeout→Failed was leaving SS_RUNNING_IMPOSSIBLE / "undefined" on the server
	// even though the service was no longer running.
	const bool stillRunning = m_processHostCompPtr.IsValid()
				&& m_processHostCompPtr->IsRunning(serviceId);
	if (!stillRunning) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited);
		return;
	}

	ApplyEvent(serviceId, CServiceFsm::Event::KillTimeout, ServiceFailureReason::StopTimeout);
}


void CServiceSupervisorComp::OnRestartBackoff()
{
	QTimer* timer = qobject_cast<QTimer*>(sender());
	if (timer == nullptr) {
		return;
	}
	const QByteArray serviceId = timer->property("serviceId").toByteArray();
	// Crashed → Starting (via Start event) + spawn. State was deliberately left Crashed
	// during the backoff so this path does not hit the Starting early-return.
	QString error;
	Start(serviceId, error);
}


void CServiceSupervisorComp::OnHealthStable()
{
	QTimer* timer = qobject_cast<QTimer*>(sender());
	if (timer == nullptr) {
		return;
	}
	const QByteArray serviceId = timer->property("serviceId").toByteArray();
	ServiceRuntimeState& state = EnsureState(serviceId);
	// Still Running with a live child → clear crash-loop budget.
	if (state.status == ServiceRuntimeStatus::Running
				&& m_processHostCompPtr.IsValid()
				&& m_processHostCompPtr->IsRunning(serviceId)) {
		state.restartCount = 0;
		state.restartWindowStart = QDateTime();
	}
}


// private methods

bool CServiceSupervisorComp::ApplyEvent(
			const QByteArray& serviceId,
			CServiceFsm::Event event,
			ServiceFailureReason forcedReason)
{
	ServiceRuntimeState& state = EnsureState(serviceId);
	const CServiceFsm::TransitionResult result = CServiceFsm::Apply(state.status, event);
	if (!result.accepted) {
		return false;
	}
	state.status = result.to;
	state.reason = (forcedReason != ServiceFailureReason::None) ? forcedReason : result.reason;
	if (state.status == ServiceRuntimeStatus::Stopped
				|| state.status == ServiceRuntimeStatus::Failed
				|| state.status == ServiceRuntimeStatus::Crashed) {
		state.pid = 0;
	}
	state.observedAt = QDateTime::currentDateTimeUtc();
	EmitStatus(state);
	return true;
}


void CServiceSupervisorComp::EmitStatus(const ServiceRuntimeState& state)
{
	// GQL subscriber path (CN_STATUS_CHANGED)
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	IServiceStatusProvider::NotifierStatusInfo info;
	info.serviceId = state.serviceId;
	info.serviceStatus = ToLegacyStatus(state.status);
	changeSet.SetChangeInfo(IServiceStatusProvider::CN_STATUS_CHANGED, QVariant::fromValue(info));
	changeSet.SetChangeInfo("serviceid", state.serviceId);
	istd::CChangeNotifier notifier(this, &changeSet);
}


ServiceRuntimeState& CServiceSupervisorComp::EnsureState(const QByteArray& serviceId)
{
	if (!m_states.contains(serviceId)) {
		ServiceRuntimeState state;
		state.serviceId = serviceId;
		state.status = ServiceRuntimeStatus::Stopped;
		state.observedAt = QDateTime::currentDateTimeUtc();
		m_states.insert(serviceId, state);
	}
	return m_states[serviceId];
}


bool CServiceSupervisorComp::BuildSpawnRequest(
			const QByteArray& serviceId,
			IProcessHost::SpawnRequest& request,
			QString& errorMessage) const
{
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)) {
		errorMessage = QStringLiteral("Service not found");
		return false;
	}
	IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
	if (info == nullptr) {
		errorMessage = QStringLiteral("Invalid service descriptor");
		return false;
	}

	request.serviceId = serviceId;
	const QByteArray startScript = info->GetStartScriptPath();
	const QByteArray path = info->GetServicePath();
	if (!startScript.isEmpty()) {
		request.program = QString::fromUtf8(startScript);
	}
	else if (!path.isEmpty()) {
		request.program = QString::fromUtf8(path);
	}
	else {
		errorMessage = QStringLiteral("Service has no executable or start script");
		return false;
	}

	const QByteArrayList args = info->GetServiceArguments();
	for (const QByteArray& arg : args) {
		request.arguments.append(QString::fromUtf8(arg));
	}
	request.workingDirectory = QFileInfo(request.program).absolutePath();
	return true;
}


RestartPolicy CServiceSupervisorComp::PolicyFor(const QByteArray& serviceId) const
{
	Q_UNUSED(serviceId);
	RestartPolicy policy;
	policy.stableSeconds = m_stableSeconds;
	return policy;
}


void CServiceSupervisorComp::CancelTimer(QHash<QByteArray, QTimer*>& timers, const QByteArray& serviceId)
{
	QTimer* timer = timers.value(serviceId, nullptr);
	if (timer != nullptr) {
		timer->stop();
	}
}


void CServiceSupervisorComp::ArmHealthTimer(const QByteArray& serviceId)
{
	const RestartPolicy policy = PolicyFor(serviceId);
	const int stableMs = qMax(1, policy.stableSeconds) * 1000;
	QTimer* timer = m_healthTimers.value(serviceId, nullptr);
	if (timer == nullptr) {
		timer = new QTimer(this);
		timer->setSingleShot(true);
		timer->setProperty("serviceId", serviceId);
		connect(timer, &QTimer::timeout, this, &CServiceSupervisorComp::OnHealthStable);
		m_healthTimers.insert(serviceId, timer);
	}
	timer->start(stableMs);
}


bool CServiceSupervisorComp::SpawnChild(const QByteArray& serviceId, QString& errorMessage)
{
	IProcessHost::SpawnRequest request;
	if (!BuildSpawnRequest(serviceId, request, errorMessage)) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited, ServiceFailureReason::SpawnError);
		return false;
	}

	qint64 pid = 0;
	if (!m_processHostCompPtr->Spawn(request, pid, errorMessage)) {
		ApplyEvent(serviceId, CServiceFsm::Event::ChildExited, ServiceFailureReason::SpawnError);
		return false;
	}

	// Already emitted Starting via ApplyEvent(Start). ChildReady → Running from OnChildStarted.
	// OnChildStarted also persists durable PID.
	ServiceRuntimeState& state = EnsureState(serviceId);
	if (pid > 0) {
		state.pid = pid;
	}
	state.observedAt = QDateTime::currentDateTimeUtc();
	return true;
}


bool CServiceSupervisorComp::AlignRunning(const QByteArray& serviceId, QString& errorMessage)
{
	ServiceRuntimeState& state = EnsureState(serviceId);
	if (state.status == ServiceRuntimeStatus::Running) {
		if (m_processHostCompPtr.IsValid()) {
			state.pid = m_processHostCompPtr->Pid(serviceId);
		}
		return true;
	}
	if (state.status == ServiceRuntimeStatus::Stopped
				|| state.status == ServiceRuntimeStatus::Crashed
				|| state.status == ServiceRuntimeStatus::Failed) {
		if (!ApplyEvent(serviceId, CServiceFsm::Event::Start)) {
			errorMessage = QStringLiteral("Invalid Start transition while aligning Running");
			return false;
		}
	}
	state = EnsureState(serviceId);
	if (state.status == ServiceRuntimeStatus::Starting) {
		if (!ApplyEvent(serviceId, CServiceFsm::Event::ChildReady)) {
			errorMessage = QStringLiteral("Invalid ChildReady while aligning Running");
			return false;
		}
	}
	state = EnsureState(serviceId);
	if (m_processHostCompPtr.IsValid()) {
		state.pid = m_processHostCompPtr->Pid(serviceId);
	}
	ArmHealthTimer(serviceId);
	return state.status == ServiceRuntimeStatus::Running;
}


bool CServiceSupervisorComp::TryAdoptFromDurableState(const QByteArray& serviceId, QString& errorMessage)
{
	if (!m_processHostCompPtr.IsValid() || !m_durablePids.contains(serviceId)) {
		return false;
	}
	const DurablePidEntry entry = m_durablePids.value(serviceId);
	if (entry.pid <= 0) {
		return false;
	}

	IProcessHost::SpawnRequest spawnReq;
	QString program = entry.program;
	QString executable = entry.executable;
	QString unused;
	if (BuildSpawnRequest(serviceId, spawnReq, unused)) {
		if (program.isEmpty()) {
			program = spawnReq.program;
		}
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (executable.isEmpty()
				&& m_serviceCollectionCompPtr.IsValid()
				&& m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)) {
		IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
		if (info != nullptr) {
			executable = QString::fromUtf8(info->GetServicePath());
		}
	}

	// Prefer executable image (real process) over start-script path for verification.
	const QStringList candidates = [&]() {
		QStringList list;
		if (!executable.isEmpty()) {
			list << executable;
		}
		if (!program.isEmpty() && program != executable) {
			list << program;
		}
		if (list.isEmpty()) {
			list << QString(); // allow live-pid adopt without path if nothing known
		}
		return list;
	}();

	IProcessHost::AdoptRequest adopt;
	adopt.serviceId = serviceId;
	adopt.pid = entry.pid;
	QString lastError;
	for (const QString& expected : candidates) {
		adopt.expectedProgram = expected;
		if (m_processHostCompPtr->TryAdopt(adopt, lastError)) {
			PersistPid(serviceId, entry.pid, program, executable);
			errorMessage.clear();
			return true;
		}
		// Failed adopt may have partially inserted — ensure clean for next candidate.
		if (m_processHostCompPtr->IsRunning(serviceId)) {
			// Should not happen on failure; leave as-is.
			errorMessage.clear();
			return true;
		}
	}

	errorMessage = lastError;
	// Stale PID / recycled by OS / path mismatch — drop so we spawn cleanly.
	ClearPersistedPid(serviceId);
	return false;
}


void CServiceSupervisorComp::PersistPid(
			const QByteArray& serviceId,
			qint64 pid,
			const QString& program,
			const QString& executable)
{
	if (serviceId.isEmpty() || pid <= 0) {
		return;
	}
	DurablePidEntry entry;
	entry.pid = pid;
	entry.program = program;
	entry.executable = executable;
	m_durablePids.insert(serviceId, entry);
	SaveDurableState();
}


void CServiceSupervisorComp::ClearPersistedPid(const QByteArray& serviceId)
{
	if (m_durablePids.remove(serviceId) > 0) {
		SaveDurableState();
	}
}


void CServiceSupervisorComp::LoadDurableState()
{
	m_durablePids.clear();
	const QString path = m_runtimeStatePath.isEmpty() ? ResolveRuntimeStatePath() : m_runtimeStatePath;
	QFile file(path);
	if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
		return;
	}
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();
	if (!doc.isObject()) {
		return;
	}
	const QJsonObject services = doc.object().value(QStringLiteral("services")).toObject();
	for (auto it = services.begin(); it != services.end(); ++it) {
		const QJsonObject obj = it.value().toObject();
		DurablePidEntry entry;
		entry.pid = obj.value(QStringLiteral("pid")).toVariant().toLongLong();
		entry.program = obj.value(QStringLiteral("program")).toString();
		entry.executable = obj.value(QStringLiteral("executable")).toString();
		if (entry.pid > 0) {
			m_durablePids.insert(it.key().toUtf8(), entry);
		}
	}
}


void CServiceSupervisorComp::SaveDurableState() const
{
	const QString path = m_runtimeStatePath.isEmpty() ? ResolveRuntimeStatePath() : m_runtimeStatePath;
	QJsonObject services;
	for (auto it = m_durablePids.constBegin(); it != m_durablePids.constEnd(); ++it) {
		QJsonObject obj;
		obj.insert(QStringLiteral("pid"), static_cast<double>(it->pid));
		obj.insert(QStringLiteral("program"), it->program);
		obj.insert(QStringLiteral("executable"), it->executable);
		services.insert(QString::fromUtf8(it.key()), obj);
	}
	QJsonObject root;
	root.insert(QStringLiteral("services"), services);
	root.insert(QStringLiteral("updatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

	const QFileInfo info(path);
	QDir().mkpath(info.absolutePath());
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	file.close();
}


QString CServiceSupervisorComp::ResolveRuntimeStatePath() const
{
	QString configured;
	if (m_runtimeStatePathAttrPtr.IsValid()) {
		configured = *m_runtimeStatePathAttrPtr;
	}
	if (configured.isEmpty()) {
		configured = QStringLiteral("ServiceRuntimePids.json");
	}
	const QFileInfo info(configured);
	if (info.isAbsolute()) {
		return info.absoluteFilePath();
	}
	// Prefer application directory (next to agent binary / ServicesSettings.xml).
	const QString appDir = QCoreApplication::applicationDirPath();
	if (!appDir.isEmpty()) {
		return QDir(appDir).filePath(configured);
	}
	const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return QDir(dataDir).filePath(configured);
}


void CServiceSupervisorComp::RefreshTypeCatalog(const QByteArray& serviceId) const
{
	if (!m_typeCatalogCompPtr.IsValid() || !m_serviceCollectionCompPtr.IsValid()) {
		return;
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)) {
		return;
	}
	IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
	if (info == nullptr) {
		return;
	}
	const QFileInfo fileInfo(QString::fromUtf8(info->GetServicePath()));
	const QString pluginDir = fileInfo.path() + QStringLiteral("/Plugins");
	const QByteArray typeId = info->GetServiceTypeId().toUtf8();
	QString error;
	m_typeCatalogCompPtr->EnsureTypeLoaded(typeId, pluginDir, fileInfo.baseName(), error);
}


IServiceStatusInfo::ServiceStatus CServiceSupervisorComp::ToLegacyStatus(ServiceRuntimeStatus status)
{
	switch (status) {
	case ServiceRuntimeStatus::Starting: return IServiceStatusInfo::SS_STARTING;
	case ServiceRuntimeStatus::Running: return IServiceStatusInfo::SS_RUNNING;
	case ServiceRuntimeStatus::Stopping: return IServiceStatusInfo::SS_STOPPING;
	case ServiceRuntimeStatus::Crashed:
	case ServiceRuntimeStatus::Failed: return IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE;
	case ServiceRuntimeStatus::Stopped:
	default: return IServiceStatusInfo::SS_NOT_RUNNING;
	}
}


void CServiceSupervisorComp::AutoStartServices()
{
	if (!m_serviceCollectionCompPtr.IsValid()) {
		return;
	}
	const imtbase::ICollectionInfo::Ids ids = m_serviceCollectionCompPtr->GetElementIds();
	for (const QByteArray& id : ids) {
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (!m_serviceCollectionCompPtr->GetObjectData(id, dataPtr)) {
			continue;
		}
		IServiceInfo* info = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
		if (info == nullptr || !info->IsAutoStart()) {
			continue;
		}
		// Start() adopts durable PID first (no table scan), else spawns.
		// Skip only if host already tracks a live child after prior Start in this loop.
		if (m_processHostCompPtr.IsValid() && m_processHostCompPtr->IsRunning(id)) {
			continue;
		}
		QString error;
		Start(id, error);
	}
}


} // namespace agentinodata
