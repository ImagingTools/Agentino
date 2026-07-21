// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CProcessHostComp.h>


// Qt includes
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

#ifdef Q_OS_WIN
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	include <windows.h>
#else
#	include <errno.h>
#	include <signal.h>
#	include <unistd.h>
#endif


namespace agentinodata
{


namespace
{


void CloseNativeHandle(void*& handle)
{
#ifdef Q_OS_WIN
	if (handle != nullptr) {
		::CloseHandle(static_cast<HANDLE>(handle));
		handle = nullptr;
	}
#else
	Q_UNUSED(handle);
	handle = nullptr;
#endif
}


} // namespace


// public methods

bool CProcessHostComp::Spawn(const SpawnRequest& request, qint64& pid, QString& errorMessage)
{
	// QProcess must be created/parented on this component's thread (main). Callers from
	// GQL workers must hop here first (CServiceSupervisorComp::StartService does).
	if (QThread::currentThread() != thread()) {
		bool ok = false;
		qint64 localPid = 0;
		QString localError;
		const bool invoked = QMetaObject::invokeMethod(
					this,
					[this, request, &ok, &localPid, &localError]() {
						ok = Spawn(request, localPid, localError);
					},
					Qt::BlockingQueuedConnection);
		pid = localPid;
		errorMessage = localError;
		if (!invoked) {
			errorMessage = QStringLiteral("Unable to invoke Spawn on ProcessHost thread");
			return false;
		}
		return ok;
	}

	pid = 0;
	if (request.serviceId.isEmpty() || request.program.isEmpty()) {
		errorMessage = QStringLiteral("Invalid spawn request");
		return false;
	}

	if (m_children.contains(request.serviceId)) {
		Child& existing = m_children[request.serviceId];
		if (ChildIsAlive(existing)) {
			errorMessage = QStringLiteral("Service already has a running child");
			return false;
		}
		ClearChildEntry(request.serviceId, false);
	}

	QProcess* process = new QProcess(this);
	process->setProperty("serviceId", request.serviceId);
	if (!request.workingDirectory.isEmpty()) {
		process->setWorkingDirectory(request.workingDirectory);
	}

	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, &CProcessHostComp::OnProcessFinished);
	connect(process, &QProcess::errorOccurred, this, &CProcessHostComp::OnProcessError);
	connect(process, &QProcess::started, this, &CProcessHostComp::OnProcessStarted);

	// Insert before start/waitForStarted: started() is emitted synchronously inside
	// waitForStarted, and OnProcessStarted must find the service in m_children to
	// emit ChildStarted (supervisor Starting → Running).
	Child child;
	child.process = process;
	child.pid = 0;
	child.adopted = false;
	m_children.insert(request.serviceId, child);

	process->start(request.program, request.arguments);
	if (!process->waitForStarted(5000)) {
		errorMessage = process->errorString();
		m_children.remove(request.serviceId);
		process->disconnect(this);
		process->deleteLater();
		return false;
	}

	const qint64 startedPid = process->processId();
	m_children[request.serviceId].pid = startedPid;
	pid = startedPid;
	return true;
}


bool CProcessHostComp::TryAdopt(const AdoptRequest& request, QString& errorMessage)
{
	if (request.serviceId.isEmpty() || request.pid <= 0) {
		errorMessage = QStringLiteral("Invalid adopt request");
		return false;
	}

	if (m_children.contains(request.serviceId) && ChildIsAlive(m_children[request.serviceId])) {
		if (m_children[request.serviceId].pid == request.pid) {
			return true; // already tracking
		}
		errorMessage = QStringLiteral("Service already has a different running child");
		return false;
	}

	if (m_children.contains(request.serviceId)) {
		ClearChildEntry(request.serviceId, false);
	}

	if (!IsPidAlive(request.pid)) {
		errorMessage = QStringLiteral("Process %1 is not running").arg(request.pid);
		return false;
	}

	if (!request.expectedProgram.isEmpty()) {
		QString imagePath;
		QString pathError;
		if (!QueryProcessImagePath(request.pid, imagePath, pathError)) {
			errorMessage = pathError.isEmpty()
						? QStringLiteral("Unable to query image path for pid %1").arg(request.pid)
						: pathError;
			return false;
		}
		if (!ImagePathMatches(imagePath, request.expectedProgram)) {
			errorMessage = QStringLiteral("PID %1 image '%2' does not match expected '%3'")
						.arg(request.pid)
						.arg(imagePath, request.expectedProgram);
			return false;
		}
	}

	Child child;
	child.pid = request.pid;
	child.adopted = true;
	child.process = nullptr;

#ifdef Q_OS_WIN
	HANDLE handle = ::OpenProcess(
				PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
				FALSE,
				static_cast<DWORD>(request.pid));
	if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
		errorMessage = QStringLiteral("OpenProcess failed for pid %1 (err %2)")
					.arg(request.pid)
					.arg(static_cast<qulonglong>(::GetLastError()));
		return false;
	}
	child.nativeHandle = handle;
#endif

	m_children.insert(request.serviceId, child);
	EnsureAdoptPollTimer();
	// Do not emit ChildStarted here — caller (supervisor) drives FSM
	// Stopped/Crashed → Starting → Running after a successful adopt.
	return true;
}


bool CProcessHostComp::SignalStop(const QByteArray& serviceId, QString& errorMessage)
{
	if (QThread::currentThread() != thread()) {
		bool ok = false;
		QString localError;
		const bool invoked = QMetaObject::invokeMethod(
					this,
					[this, serviceId, &ok, &localError]() {
						ok = SignalStop(serviceId, localError);
					},
					Qt::BlockingQueuedConnection);
		errorMessage = localError;
		return invoked && ok;
	}

	if (!m_children.contains(serviceId)) {
		errorMessage = QStringLiteral("No child process for service");
		return false;
	}
	Child& child = m_children[serviceId];
	if (child.adopted) {
		return TerminateOsProcess(child, false, errorMessage);
	}
	if (child.process == nullptr) {
		errorMessage = QStringLiteral("No child process for service");
		return false;
	}
	if (child.process->state() == QProcess::NotRunning) {
		return true;
	}
	// Graceful request first; many Windows services ignore terminate() and need kill().
	child.process->terminate();
	if (!child.process->waitForFinished(3000)) {
		child.process->kill();
		child.process->waitForFinished(2000);
	}
	return true;
}


bool CProcessHostComp::ForceKill(const QByteArray& serviceId, QString& errorMessage)
{
	if (QThread::currentThread() != thread()) {
		bool ok = false;
		QString localError;
		const bool invoked = QMetaObject::invokeMethod(
					this,
					[this, serviceId, &ok, &localError]() {
						ok = ForceKill(serviceId, localError);
					},
					Qt::BlockingQueuedConnection);
		errorMessage = localError;
		return invoked && ok;
	}

	if (!m_children.contains(serviceId)) {
		errorMessage = QStringLiteral("No child process for service");
		return false;
	}
	Child& child = m_children[serviceId];
	if (child.adopted) {
		return TerminateOsProcess(child, true, errorMessage);
	}
	if (child.process == nullptr) {
		errorMessage = QStringLiteral("No child process for service");
		return false;
	}
	if (child.process->state() != QProcess::NotRunning) {
		child.process->kill();
	}
	return true;
}


bool CProcessHostComp::IsRunning(const QByteArray& serviceId) const
{
	if (!m_children.contains(serviceId)) {
		return false;
	}
	return ChildIsAlive(m_children[serviceId]);
}


qint64 CProcessHostComp::Pid(const QByteArray& serviceId) const
{
	if (!m_children.contains(serviceId)) {
		return 0;
	}
	return m_children[serviceId].pid;
}


void CProcessHostComp::Release(const QByteArray& serviceId)
{
	// Do not kill adopted processes on release (may re-bind after agent restart).
	// Spawned QProcess children are agent-owned and are killed if still running.
	ClearChildEntry(serviceId, /*killSpawnedIfRunning=*/true);
}


// protected methods

void CProcessHostComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();
	if (m_adoptPollMsAttrPtr.IsValid() && *m_adoptPollMsAttrPtr > 0) {
		m_adoptPollMs = *m_adoptPollMsAttrPtr;
	}
}


void CProcessHostComp::OnComponentDestroyed()
{
	if (m_adoptPollTimer != nullptr) {
		m_adoptPollTimer->stop();
	}
	const QList<QByteArray> ids = m_children.keys();
	for (const QByteArray& id : ids) {
		// Agent shutdown: leave adopted OS processes alone (durable PID can re-adopt).
		// Kill only QProcess-spawned children that are still running.
		ClearChildEntry(id, /*killSpawnedIfRunning=*/true);
	}
	BaseClass::OnComponentDestroyed();
}


// private slots

void CProcessHostComp::OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	const QByteArray serviceId = ServiceIdForSender();
	if (serviceId.isEmpty()) {
		return;
	}
	if (m_children.contains(serviceId)) {
		m_children[serviceId].pid = 0;
	}
	emit ChildExited(serviceId, exitCode, exitStatus);
}


void CProcessHostComp::OnProcessError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
	const QByteArray serviceId = ServiceIdForSender();
	if (serviceId.isEmpty()) {
		return;
	}
	QProcess* process = qobject_cast<QProcess*>(sender());
	const QString message = process != nullptr ? process->errorString() : QStringLiteral("Process error");
	emit ChildError(serviceId, message);
}


void CProcessHostComp::OnProcessStarted()
{
	const QByteArray serviceId = ServiceIdForSender();
	if (serviceId.isEmpty()) {
		return;
	}
	QProcess* process = qobject_cast<QProcess*>(sender());
	if (process != nullptr && m_children.contains(serviceId)) {
		m_children[serviceId].pid = process->processId();
		emit ChildStarted(serviceId, m_children[serviceId].pid);
	}
}


void CProcessHostComp::OnAdoptPoll()
{
	QList<QByteArray> dead;
	for (auto it = m_children.constBegin(); it != m_children.constEnd(); ++it) {
		if (it->adopted && !ChildIsAlive(*it)) {
			dead.append(it.key());
		}
	}
	for (const QByteArray& serviceId : dead) {
		ClearChildEntry(serviceId, false);
		emit ChildExited(serviceId, -1, QProcess::CrashExit);
	}
	// Stop poll when nothing adopted remains.
	bool anyAdopted = false;
	for (auto it = m_children.constBegin(); it != m_children.constEnd(); ++it) {
		if (it->adopted) {
			anyAdopted = true;
			break;
		}
	}
	if (!anyAdopted && m_adoptPollTimer != nullptr) {
		m_adoptPollTimer->stop();
	}
}


// private methods

QByteArray CProcessHostComp::ServiceIdForSender() const
{
	QProcess* process = qobject_cast<QProcess*>(sender());
	if (process == nullptr) {
		return {};
	}
	return process->property("serviceId").toByteArray();
}


void CProcessHostComp::EnsureAdoptPollTimer()
{
	if (m_adoptPollTimer == nullptr) {
		m_adoptPollTimer = new QTimer(this);
		connect(m_adoptPollTimer, &QTimer::timeout, this, &CProcessHostComp::OnAdoptPoll);
	}
	if (!m_adoptPollTimer->isActive()) {
		m_adoptPollTimer->start(qMax(200, m_adoptPollMs));
	}
}


void CProcessHostComp::ClearChildEntry(const QByteArray& serviceId, bool killSpawnedIfRunning)
{
	if (!m_children.contains(serviceId)) {
		return;
	}
	Child child = m_children.take(serviceId);
	if (child.process != nullptr) {
		child.process->disconnect(this);
		if (killSpawnedIfRunning && child.process->state() != QProcess::NotRunning) {
			child.process->kill();
			child.process->waitForFinished(1000);
		}
		child.process->deleteLater();
	}
	CloseNativeHandle(child.nativeHandle);
}


bool CProcessHostComp::ChildIsAlive(const Child& child) const
{
	if (child.adopted) {
		return child.pid > 0 && IsPidAlive(child.pid);
	}
	if (child.process == nullptr) {
		return false;
	}
	return child.process->state() != QProcess::NotRunning;
}


bool CProcessHostComp::TerminateOsProcess(const Child& child, bool force, QString& errorMessage) const
{
	if (child.pid <= 0) {
		errorMessage = QStringLiteral("No pid for adopted child");
		return false;
	}
#ifdef Q_OS_WIN
	HANDLE handle = static_cast<HANDLE>(child.nativeHandle);
	HANDLE opened = nullptr;
	if (handle == nullptr) {
		opened = ::OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(child.pid));
		handle = opened;
	}
	if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
		errorMessage = QStringLiteral("Unable to open process %1 for terminate").arg(child.pid);
		return false;
	}
	// Graceful terminate is not reliable for arbitrary GUI/service processes without
	// a custom protocol; ForceKill and SignalStop both end the process by PID.
	Q_UNUSED(force);
	const BOOL ok = ::TerminateProcess(handle, 1);
	if (opened != nullptr) {
		::CloseHandle(opened);
	}
	if (!ok) {
		errorMessage = QStringLiteral("TerminateProcess failed for pid %1").arg(child.pid);
		return false;
	}
	return true;
#else
	const int sig = force ? SIGKILL : SIGTERM;
	if (::kill(static_cast<pid_t>(child.pid), sig) != 0) {
		if (errno == ESRCH) {
			return true;
		}
		errorMessage = QStringLiteral("kill(%1) failed: %2").arg(child.pid).arg(errno);
		return false;
	}
	return true;
#endif
}


bool CProcessHostComp::IsPidAlive(qint64 pid)
{
	if (pid <= 0) {
		return false;
	}
#ifdef Q_OS_WIN
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
	if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD exitCode = 0;
	const BOOL ok = ::GetExitCodeProcess(handle, &exitCode);
	::CloseHandle(handle);
	return ok && exitCode == STILL_ACTIVE;
#else
	return ::kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
#endif
}


bool CProcessHostComp::QueryProcessImagePath(qint64 pid, QString& imagePath, QString& errorMessage)
{
	imagePath.clear();
	if (pid <= 0) {
		errorMessage = QStringLiteral("Invalid pid");
		return false;
	}
#ifdef Q_OS_WIN
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
	if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
		errorMessage = QStringLiteral("OpenProcess(query) failed for pid %1").arg(pid);
		return false;
	}
	wchar_t buffer[MAX_PATH * 4];
	DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
	const BOOL ok = ::QueryFullProcessImageNameW(handle, 0, buffer, &size);
	::CloseHandle(handle);
	if (!ok) {
		errorMessage = QStringLiteral("QueryFullProcessImageName failed for pid %1").arg(pid);
		return false;
	}
	imagePath = QString::fromWCharArray(buffer, static_cast<int>(size));
	return true;
#else
	const QString link = QStringLiteral("/proc/%1/exe").arg(pid);
	const QByteArray target = QFile::symLinkTarget(link).toUtf8();
	if (target.isEmpty()) {
		// Fallback: readlink
		char buf[4096];
		const ssize_t n = ::readlink(link.toUtf8().constData(), buf, sizeof(buf) - 1);
		if (n <= 0) {
			errorMessage = QStringLiteral("Unable to resolve %1").arg(link);
			return false;
		}
		buf[n] = '\0';
		imagePath = QString::fromUtf8(buf);
		return true;
	}
	imagePath = QString::fromUtf8(target);
	return true;
#endif
}


bool CProcessHostComp::ImagePathMatches(const QString& actualPath, const QString& expectedProgram)
{
	if (expectedProgram.isEmpty()) {
		return true;
	}
	const QFileInfo actualInfo(actualPath);
	const QFileInfo expectedInfo(expectedProgram);
	const QString actualCanon = actualInfo.canonicalFilePath();
	const QString expectedCanon = expectedInfo.canonicalFilePath();
	if (!actualCanon.isEmpty() && !expectedCanon.isEmpty()) {
		return QString::compare(actualCanon, expectedCanon, Qt::CaseInsensitive) == 0;
	}
	// Basename fallback (path moved / not fully resolvable).
	return QString::compare(actualInfo.fileName(), expectedInfo.fileName(), Qt::CaseInsensitive) == 0;
}


} // namespace agentinodata
