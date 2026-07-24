// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CTerminalSessionManagerComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QThread>
#include <QtCore/QUuid>

// ACF includes
#include <istd/CChangeNotifier.h>

#if defined(Q_OS_WIN)
#	include <cstdlib>
#else
#	include <errno.h>
#	include <fcntl.h>
#	include <signal.h>
#	include <string.h>
#	include <sys/ioctl.h>
#	include <termios.h>
#	include <unistd.h>
#	if defined(Q_OS_MAC)
#		include <util.h>
#	else
#		include <pty.h>
#	endif
#endif


namespace agentinodata
{


#if defined(Q_OS_WIN)

// public methods (CTerminalPtyReaderThread)

CTerminalPtyReaderThread::CTerminalPtyReaderThread(HANDLE pipeReadHandle, QObject* parentPtr)
	:QThread(parentPtr)
	,m_pipeReadHandle(pipeReadHandle)
{
}


// protected methods (CTerminalPtyReaderThread)

void CTerminalPtyReaderThread::run()
{
	char buffer[4096];

	// Blocks until the pipe has data, the child writes, or the owner closes
	// m_pipeReadHandle (ReadFile then fails with ERROR_BROKEN_PIPE - our cue to stop).
	// A TRUE result with 0 bytes is not a documented ConPTY/anonymous-pipe outcome for
	// a blocking handle, but is treated as "nothing to emit yet, keep waiting" rather
	// than as end-of-stream, so it can never be mistaken for the pipe having closed.
	for (;;){
		DWORD bytesRead = 0;
		const BOOL ok = ::ReadFile(m_pipeReadHandle, buffer, sizeof(buffer), &bytesRead, nullptr);
		if (!ok){
			break;
		}
		if (bytesRead > 0){
			Q_EMIT DataRead(QByteArray(buffer, int(bytesRead)));
		}
	}

	Q_EMIT ReaderFinished();
}

#endif // Q_OS_WIN


// public methods

// reimplemented (agentinodata::ITerminalController)

QList<ITerminalController::ShellInfo> CTerminalSessionManagerComp::GetAvailableShells() const
{
	QList<ShellInfo> retVal;

	const ShellType candidates[] = {ST_CMD, ST_POWERSHELL, ST_BASH, ST_SH};
	for (ShellType shellType: candidates){
		QString program;
		QStringList arguments;

		ShellInfo info;
		info.type = shellType;
		info.available = ResolveShellProgram(shellType, program, arguments);

		switch (shellType){
		case ST_CMD:
			info.name = QStringLiteral("Command Prompt");
			break;
		case ST_POWERSHELL:
			info.name = QStringLiteral("PowerShell");
			break;
		case ST_BASH:
			info.name = QStringLiteral("Bash");
			break;
		case ST_SH:
			info.name = QStringLiteral("Shell");
			break;
		}

		retVal.append(info);
	}

	return retVal;
}


QByteArray CTerminalSessionManagerComp::OpenSession(ShellType shellType, QString& errorMessage)
{
	QByteArray retVal;

	RunOnComponentThread([&](){
		retVal = OpenSessionOnOwnThread(shellType, errorMessage);
	});

	return retVal;
}


bool CTerminalSessionManagerComp::SendInput(const QByteArray& sessionId, const QString& data)
{
	bool retVal = false;

	RunOnComponentThread([&](){
		retVal = SendInputOnOwnThread(sessionId, data);
	});

	return retVal;
}


bool CTerminalSessionManagerComp::CloseSession(const QByteArray& sessionId)
{
	bool retVal = false;

	RunOnComponentThread([&](){
		QMutexLocker locker(&m_mutex);

		if (!m_sessionMap.contains(sessionId)){
			return;
		}

		RemoveSession(sessionId);
		retVal = true;
	});

	if (retVal){
		SendInfoMessage(0, QString("Terminal session '%1' closed").arg(QString(sessionId)), "CTerminalSessionManagerComp");
	}

	return retVal;
}


bool CTerminalSessionManagerComp::InterruptSession(const QByteArray& sessionId)
{
	bool retVal = false;

	RunOnComponentThread([&](){
		retVal = InterruptSessionOnOwnThread(sessionId);
	});

	return retVal;
}


bool CTerminalSessionManagerComp::ResizeSession(const QByteArray& sessionId, int columns, int rows)
{
	bool retVal = false;

	RunOnComponentThread([&](){
		retVal = ResizeSessionOnOwnThread(sessionId, columns, rows);
	});

	return retVal;
}


QList<ITerminalController::OutputChunk> CTerminalSessionManagerComp::GetOutput(
			const QByteArray& sessionId,
			qint64 fromSequence,
			qint64& nextSequence,
			bool& running,
			int& exitCode) const
{
	QList<OutputChunk> retVal;

	nextSequence = fromSequence;
	running = false;
	exitCode = -1;

	QMutexLocker locker(&m_mutex);

	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		return retVal;
	}

	running = !sessionPtr->finished;
	exitCode = sessionPtr->exitCode;

	for (const OutputChunk& chunk: sessionPtr->chunks){
		if (chunk.sequence >= fromSequence){
			retVal.append(chunk);
		}
	}

	nextSequence = sessionPtr->nextSequence;

	// Idle lifetime is driven by real activity (SendInput / process I/O), not by
	// GetOutput - the publisher and catch-up reads must not keep a session alive.

	return retVal;
}


bool CTerminalSessionManagerComp::SessionExists(const QByteArray& sessionId) const
{
	QMutexLocker locker(&m_mutex);

	return m_sessionMap.contains(sessionId);
}


// reimplemented (icomp::CComponentBase)

void CTerminalSessionManagerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	m_idleTimer.setInterval(IdleCheckIntervalMs);
	connect(&m_idleTimer, &QTimer::timeout, this, &CTerminalSessionManagerComp::OnIdleTimeout);
	m_idleTimer.start();
}


void CTerminalSessionManagerComp::OnComponentDestroyed()
{
	m_idleTimer.stop();

	QMutexLocker locker(&m_mutex);

	const QList<QByteArray> sessionIds = m_sessionMap.keys();
	for (const QByteArray& sessionId: sessionIds){
		RemoveSession(sessionId);
	}

	locker.unlock();

	BaseClass::OnComponentDestroyed();
}


// protected slots

void CTerminalSessionManagerComp::OnIdleTimeout()
{
	const QDateTime now = QDateTime::currentDateTimeUtc();

	QMutexLocker locker(&m_mutex);

	const QList<QByteArray> sessionIds = m_sessionMap.keys();
	for (const QByteArray& sessionId: sessionIds){
		Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (sessionPtr == nullptr || sessionPtr->finished){
			continue;
		}

		const qint64 idleSeconds = sessionPtr->lastActivity.secsTo(now);
		if (idleSeconds < 0){
			continue;
		}

		// ~60s before close: push a SYSTEM warning (does not refresh lastActivity).
		if (!sessionPtr->idleWarningSent
					&& idleSeconds >= IdleTimeoutSeconds - IdleWarningLeadSeconds
					&& idleSeconds < IdleTimeoutSeconds){
			sessionPtr->idleWarningSent = true;
			AppendChunk(
						sessionId,
						*sessionPtr,
						STREAM_SYSTEM,
						QString("Warning: session will close in about %1 seconds due to inactivity")
									.arg(IdleWarningLeadSeconds),
						false);

			continue;
		}

		if (idleSeconds < IdleTimeoutSeconds){
			continue;
		}

		// Mark finished and push a SYSTEM chunk so GraphQL subscribers learn the
		// session is gone (RemoveSession alone would leave the GUI hanging on push).
		// Exit code -2 distinguishes idle auto-close from a manual CloseSession (-1).
		sessionPtr->finished = true;
		sessionPtr->exitCode = IdleCloseExitCode;
		AppendChunk(
					sessionId,
					*sessionPtr,
					STREAM_SYSTEM,
					QString("Session closed after %1 minutes of inactivity").arg(IdleTimeoutSeconds / 60),
					false);

		SendInfoMessage(0, QString("Terminal session '%1' closed after being idle").arg(QString(sessionId)), "CTerminalSessionManagerComp");
		RemoveSession(sessionId);
	}
}


// private methods

template <class Func>
void CTerminalSessionManagerComp::RunOnComponentThread(Func func)
{
	if (QThread::currentThread() == thread()){
		func();

		return;
	}

	// Worker path: the shell/pty handles belong to this component's thread, which is
	// inside the application event loop while workers serve requests, so blocking
	// here cannot dead-lock (same reasoning as imtcom::CRequestSender).
	QMetaObject::invokeMethod(this, func, Qt::BlockingQueuedConnection);
}


#if defined(Q_OS_WIN)

// ─── Windows: ConPTY ────────────────────────────────────────────────────────

QByteArray CTerminalSessionManagerComp::OpenSessionOnOwnThread(ShellType shellType, QString& errorMessage)
{
	QMutexLocker locker(&m_mutex);

	if (m_sessionMap.count() >= MaxSessionCount){
		errorMessage = QString("Unable to open terminal session: maximum number of sessions (%1) reached").arg(MaxSessionCount);
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	QString program;
	QStringList arguments;
	if (!ResolveShellProgram(shellType, program, arguments)){
		errorMessage = QString("Unable to open terminal session: requested shell is not available on this machine");
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	SECURITY_ATTRIBUTES pipeSecurity{};
	pipeSecurity.nLength = sizeof(pipeSecurity);
	pipeSecurity.bInheritHandle = FALSE;

	HANDLE pipeInRead = nullptr;
	HANDLE pipeInWrite = nullptr;
	if (!::CreatePipe(&pipeInRead, &pipeInWrite, &pipeSecurity, 0)){
		errorMessage = QStringLiteral("Unable to open terminal session: CreatePipe (stdin) failed");
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	HANDLE pipeOutRead = nullptr;
	HANDLE pipeOutWrite = nullptr;
	if (!::CreatePipe(&pipeOutRead, &pipeOutWrite, &pipeSecurity, 0)){
		::CloseHandle(pipeInRead);
		::CloseHandle(pipeInWrite);
		errorMessage = QStringLiteral("Unable to open terminal session: CreatePipe (stdout) failed");
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	HPCON hPC = nullptr;
	const COORD initialSize{80, 24};
	const HRESULT createResult = ::CreatePseudoConsole(initialSize, pipeInRead, pipeOutWrite, 0, &hPC);
	// ConPTY duplicated the ends it needs; our copies are no longer required either way.
	::CloseHandle(pipeInRead);
	::CloseHandle(pipeOutWrite);
	if (FAILED(createResult)){
		::CloseHandle(pipeInWrite);
		::CloseHandle(pipeOutRead);
		errorMessage = QString("Unable to open terminal session: CreatePseudoConsole failed (0x%1)")
					.arg(quint32(createResult), 8, 16, QLatin1Char('0'));
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	STARTUPINFOEXW startupInfo{};
	startupInfo.StartupInfo.cb = sizeof(startupInfo);

	SIZE_T attributeListSize = 0;
	::InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeListSize);
	startupInfo.lpAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(std::malloc(attributeListSize));

	const bool startupInfoReady = startupInfo.lpAttributeList != nullptr
				&& ::InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attributeListSize)
				&& ::UpdateProcThreadAttribute(
							startupInfo.lpAttributeList,
							0,
							PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
							hPC,
							sizeof(HPCON),
							nullptr,
							nullptr);
	if (!startupInfoReady){
		if (startupInfo.lpAttributeList != nullptr){
			::DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
			std::free(startupInfo.lpAttributeList);
		}
		::ClosePseudoConsole(hPC);
		::CloseHandle(pipeInWrite);
		::CloseHandle(pipeOutRead);
		errorMessage = QStringLiteral("Unable to open terminal session: failed to prepare the pseudo console attribute list");
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	// Arguments here are fixed flags chosen by ResolveShellProgram, never user input,
	// so this simple quoting is safe (no embedded quotes/spaces to escape).
	QString commandLine = QStringLiteral("\"%1\"").arg(program);
	for (const QString& argument: arguments){
		commandLine += QStringLiteral(" \"%1\"").arg(argument);
	}
	std::wstring commandLineBuffer = commandLine.toStdWString();
	commandLineBuffer.push_back(L'\0'); // CreateProcessW requires a mutable, writable buffer

	const std::wstring workingDirectory = QDir::toNativeSeparators(QDir::homePath()).toStdWString();

	PROCESS_INFORMATION processInfo{};
	const BOOL created = ::CreateProcessW(
				nullptr,
				commandLineBuffer.data(),
				nullptr,
				nullptr,
				FALSE,
				EXTENDED_STARTUPINFO_PRESENT,
				nullptr,
				workingDirectory.c_str(),
				&startupInfo.StartupInfo,
				&processInfo);

	::DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
	std::free(startupInfo.lpAttributeList);

	if (!created){
		::ClosePseudoConsole(hPC);
		::CloseHandle(pipeInWrite);
		::CloseHandle(pipeOutRead);
		errorMessage = QString("Unable to start shell '%1': Win32 error %2").arg(program).arg(::GetLastError());
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	::CloseHandle(processInfo.hThread); // only the process handle is needed going forward

	const QByteArray sessionId = QByteArrayLiteral("term-")
				+ QByteArray::number(++m_sessionCounter)
				+ '-'
				+ QUuid::createUuid().toByteArray(QUuid::Id128);

	istd::TDelPtr<Session> sessionPtr(new Session);
	sessionPtr->shellType = shellType;
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->hPC = hPC;
	sessionPtr->processInfo = processInfo;
	sessionPtr->pipeInWrite = pipeInWrite;
	sessionPtr->pipeOutRead = pipeOutRead;

	Session* rawSessionPtr = sessionPtr.PopPtr();
	m_sessionMap.insert(sessionId, rawSessionPtr);

	// Every handler below re-looks-up the session by id (never captures the raw
	// pointer): once RemoveSession() takes it out of m_sessionMap, any handler still
	// queued for a since-removed session safely no-ops instead of touching freed memory.

	rawSessionPtr->readerThreadPtr.SetPtr(new CTerminalPtyReaderThread(pipeOutRead, this));
	connect(rawSessionPtr->readerThreadPtr.GetPtr(), &CTerminalPtyReaderThread::DataRead, this,
				[this, sessionId](const QByteArray& data){
		QMutexLocker dataLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr){
			return;
		}
		const QString decoded = liveSessionPtr->ptyDecoder.decode(data);
		if (!decoded.isEmpty()){
			AppendChunk(sessionId, *liveSessionPtr, STREAM_STDOUT, decoded);
		}
	});
	connect(rawSessionPtr->readerThreadPtr.GetPtr(), &CTerminalPtyReaderThread::ReaderFinished, this,
				[this, sessionId](){
		QMutexLocker finishLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr){
			return;
		}
		liveSessionPtr->readerFinished = true;
		FinalizeIfDone(sessionId, *liveSessionPtr, liveSessionPtr->exitCode);
	});
	rawSessionPtr->readerThreadPtr->start();

	rawSessionPtr->exitNotifierPtr.SetPtr(new QWinEventNotifier(processInfo.hProcess, this));
	connect(rawSessionPtr->exitNotifierPtr.GetPtr(), &QWinEventNotifier::activated, this,
				[this, sessionId](){
		QMutexLocker exitLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr){
			return;
		}
		DWORD winExitCode = 0;
		::GetExitCodeProcess(liveSessionPtr->processInfo.hProcess, &winExitCode);
		liveSessionPtr->processExited = true;
		FinalizeIfDone(sessionId, *liveSessionPtr, int(winExitCode));
	});

	AppendChunk(sessionId, *rawSessionPtr, STREAM_SYSTEM, QString("Session started (%1)").arg(program));

	SendInfoMessage(0, QString("Terminal session '%1' started (%2)").arg(QString(sessionId), program), "CTerminalSessionManagerComp");

	return sessionId;
}


void CTerminalSessionManagerComp::RemoveSession(const QByteArray& sessionId)
{
	Session* sessionPtr = m_sessionMap.take(sessionId);
	if (sessionPtr == nullptr){
		return;
	}

	if (sessionPtr->exitNotifierPtr.IsValid()){
		sessionPtr->exitNotifierPtr->setEnabled(false);
		sessionPtr->exitNotifierPtr.SetPtr(nullptr);
	}

	if (sessionPtr->hPC != nullptr){
		// Hangs up the child's console I/O (typically enough to end an interactive shell).
		::ClosePseudoConsole(sessionPtr->hPC);
		sessionPtr->hPC = nullptr;
	}
	if (sessionPtr->pipeInWrite != nullptr){
		::CloseHandle(sessionPtr->pipeInWrite);
		sessionPtr->pipeInWrite = nullptr;
	}
	if (sessionPtr->pipeOutRead != nullptr){
		// Breaks the reader thread's pending ReadFile (ERROR_BROKEN_PIPE) so it exits.
		::CloseHandle(sessionPtr->pipeOutRead);
		sessionPtr->pipeOutRead = nullptr;
	}
	if (sessionPtr->readerThreadPtr.IsValid()){
		sessionPtr->readerThreadPtr->wait(2000);
		sessionPtr->readerThreadPtr.SetPtr(nullptr);
	}

	if (sessionPtr->processInfo.hProcess != nullptr){
		DWORD winExitCode = STILL_ACTIVE;
		::GetExitCodeProcess(sessionPtr->processInfo.hProcess, &winExitCode);
		if (winExitCode == STILL_ACTIVE){
			// ConPTY teardown above did not end it in time - hard fallback.
			::TerminateProcess(sessionPtr->processInfo.hProcess, 1);
		}
		::CloseHandle(sessionPtr->processInfo.hProcess);
		sessionPtr->processInfo.hProcess = nullptr;
	}

	delete sessionPtr;
}

#else // Unix (Linux/macOS): openpty + QProcess::setChildProcessModifier

// ─── Unix: openpty ──────────────────────────────────────────────────────────

QByteArray CTerminalSessionManagerComp::OpenSessionOnOwnThread(ShellType shellType, QString& errorMessage)
{
	QMutexLocker locker(&m_mutex);

	if (m_sessionMap.count() >= MaxSessionCount){
		errorMessage = QString("Unable to open terminal session: maximum number of sessions (%1) reached").arg(MaxSessionCount);
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	QString program;
	QStringList arguments;
	if (!ResolveShellProgram(shellType, program, arguments)){
		errorMessage = QString("Unable to open terminal session: requested shell is not available on this machine");
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	int masterFd = -1;
	int slaveFd = -1;
	if (::openpty(&masterFd, &slaveFd, nullptr, nullptr, nullptr) != 0){
		errorMessage = QString("Unable to open terminal session: openpty failed (%1)").arg(QString::fromLocal8Bit(::strerror(errno)));
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}
	// Never let the master survive into the forked child (only the slave should).
	::fcntl(masterFd, F_SETFD, FD_CLOEXEC);

	const QByteArray sessionId = QByteArrayLiteral("term-")
				+ QByteArray::number(++m_sessionCounter)
				+ '-'
				+ QUuid::createUuid().toByteArray(QUuid::Id128);

	istd::TDelPtr<Session> sessionPtr(new Session);
	sessionPtr->shellType = shellType;
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->processPtr.SetPtr(new QProcess(this));

	QProcess* processPtr = sessionPtr->processPtr.GetPtr();
	processPtr->setProgram(program);
	processPtr->setArguments(arguments);
	processPtr->setWorkingDirectory(QDir::homePath());

	// Runs IN the forked child, after fork()/before exec() - async-signal-safe calls
	// only (no Qt, no heap allocation). slaveFd is a plain int, captured by value.
	processPtr->setChildProcessModifier([slaveFd](){
		::setsid();
		::ioctl(slaveFd, TIOCSCTTY, 0);
		::dup2(slaveFd, STDIN_FILENO);
		::dup2(slaveFd, STDOUT_FILENO);
		::dup2(slaveFd, STDERR_FILENO);
		if (slaveFd > STDERR_FILENO){
			::close(slaveFd);
		}
	});

	Session* rawSessionPtr = sessionPtr.PopPtr();
	m_sessionMap.insert(sessionId, rawSessionPtr);

	// Handlers re-look-up the session by id rather than capturing the raw pointer, so a
	// signal still queued for a since-removed session safely no-ops (see RemoveSession).
	connect(processPtr, &QProcess::finished, this,
				[this, sessionId](int exitCode, QProcess::ExitStatus exitStatus){
		Q_UNUSED(exitStatus);
		QMutexLocker finishLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr){
			return;
		}
		FinalizeIfDone(sessionId, *liveSessionPtr, exitCode);
	});
	connect(processPtr, &QProcess::errorOccurred, this,
				[this, sessionId](QProcess::ProcessError error){
		Q_UNUSED(error);
		QMutexLocker errorLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr || !liveSessionPtr->processPtr.IsValid()){
			return;
		}
		AppendChunk(sessionId, *liveSessionPtr, STREAM_SYSTEM,
					QString("Process error: %1").arg(liveSessionPtr->processPtr->errorString()));
	});

	processPtr->start(QIODevice::ReadWrite);
	if (!processPtr->waitForStarted(5000)){
		errorMessage = QString("Unable to start shell '%1': %2").arg(program, processPtr->errorString());
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		m_sessionMap.remove(sessionId);
		processPtr->disconnect(this);
		delete rawSessionPtr;
		::close(masterFd);
		::close(slaveFd);

		return QByteArray();
	}

	::close(slaveFd); // the child dup'd its own copy; our parent-side copy is no longer needed

	rawSessionPtr->ptyMasterFd = masterFd;
	rawSessionPtr->readNotifierPtr.SetPtr(new QSocketNotifier(masterFd, QSocketNotifier::Read, this));
	connect(rawSessionPtr->readNotifierPtr.GetPtr(), &QSocketNotifier::activated, this,
				[this, sessionId](){
		QMutexLocker readLocker(&m_mutex);
		Session* liveSessionPtr = m_sessionMap.value(sessionId, nullptr);
		if (liveSessionPtr == nullptr){
			return;
		}
		char buffer[4096];
		const ssize_t bytesRead = ::read(liveSessionPtr->ptyMasterFd, buffer, sizeof(buffer));
		if (bytesRead > 0){
			const QString decoded = liveSessionPtr->ptyDecoder.decode(QByteArray(buffer, int(bytesRead)));
			if (!decoded.isEmpty()){
				AppendChunk(sessionId, *liveSessionPtr, STREAM_STDOUT, decoded);
			}
		}
	});

	AppendChunk(sessionId, *rawSessionPtr, STREAM_SYSTEM, QString("Session started (%1)").arg(program));

	SendInfoMessage(0, QString("Terminal session '%1' started (%2)").arg(QString(sessionId), program), "CTerminalSessionManagerComp");

	return sessionId;
}


void CTerminalSessionManagerComp::RemoveSession(const QByteArray& sessionId)
{
	Session* sessionPtr = m_sessionMap.take(sessionId);
	if (sessionPtr == nullptr){
		return;
	}

	// Delete/disable the notifier before closing the fd it wraps (Qt requirement).
	if (sessionPtr->readNotifierPtr.IsValid()){
		sessionPtr->readNotifierPtr->setEnabled(false);
		sessionPtr->readNotifierPtr.SetPtr(nullptr);
	}
	if (sessionPtr->ptyMasterFd >= 0){
		// Closing the master typically delivers SIGHUP to the session as its
		// controlling terminal goes away.
		::close(sessionPtr->ptyMasterFd);
		sessionPtr->ptyMasterFd = -1;
	}

	if (sessionPtr->processPtr.IsValid()){
		QProcess* processPtr = sessionPtr->processPtr.GetPtr();
		processPtr->disconnect(this);
		if (processPtr->state() != QProcess::NotRunning){
			processPtr->terminate();
			if (!processPtr->waitForFinished(2000)){
				processPtr->kill();
				processPtr->waitForFinished(2000);
			}
		}
	}

	delete sessionPtr;
}

#endif // platform split


// ─── Shared (both platforms) ────────────────────────────────────────────────

bool CTerminalSessionManagerComp::WriteRawOnOwnThread(Session& session, const QByteArray& bytes)
{
	if (session.finished){
		return false;
	}

#if defined(Q_OS_WIN)
	if (session.pipeInWrite == nullptr){
		return false;
	}
	DWORD written = 0;
	const BOOL ok = ::WriteFile(session.pipeInWrite, bytes.constData(), DWORD(bytes.size()), &written, nullptr);

	return ok && written == DWORD(bytes.size());
#else
	if (session.ptyMasterFd < 0){
		return false;
	}
	const ssize_t written = ::write(session.ptyMasterFd, bytes.constData(), size_t(bytes.size()));

	return written == ssize_t(bytes.size());
#endif
}


bool CTerminalSessionManagerComp::SendInputOnOwnThread(const QByteArray& sessionId, const QString& data)
{
	QMutexLocker locker(&m_mutex);

	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' does not exist").arg(QString(sessionId)), "CTerminalSessionManagerComp");

		return false;
	}

	if (data.length() > MaxInputLength){
		SendErrorMessage(0, QString("Unable to send input: request exceeds the maximum allowed length (%1)").arg(MaxInputLength), "CTerminalSessionManagerComp");

		return false;
	}

	if (sessionPtr->finished){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' is no longer running").arg(QString(sessionId)), "CTerminalSessionManagerComp");

		return false;
	}

	// The user input is written verbatim to the pty. A trailing new line is added only
	// when missing so the typed command is actually executed by the shell.
	QString payload = data;
	if (!payload.endsWith('\n')){
		payload.append('\n');
	}

	const bool written = WriteRawOnOwnThread(*sessionPtr, payload.toUtf8());
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->idleWarningSent = false;

	return written;
}


bool CTerminalSessionManagerComp::InterruptSessionOnOwnThread(const QByteArray& sessionId)
{
	QMutexLocker locker(&m_mutex);

	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr || sessionPtr->finished){
		return false;
	}

	// A real pty/ConPTY turns this single byte into SIGINT / CTRL_C_EVENT for the
	// foreground process group - the same mechanism a real terminal uses, so no
	// separate signal-delivery code is needed on either platform.
	static const char ctrlC = 0x03;
	const bool written = WriteRawOnOwnThread(*sessionPtr, QByteArray(&ctrlC, 1));
	if (written){
		sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
		sessionPtr->idleWarningSent = false;
		AppendChunk(sessionId, *sessionPtr, STREAM_SYSTEM, QStringLiteral("^C"), false);
	}

	return written;
}


bool CTerminalSessionManagerComp::ResizeSessionOnOwnThread(const QByteArray& sessionId, int columns, int rows)
{
	QMutexLocker locker(&m_mutex);

	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr || sessionPtr->finished){
		return false;
	}

#if defined(Q_OS_WIN)
	if (sessionPtr->hPC == nullptr){
		return false;
	}
	const COORD size{SHORT(qBound(1, columns, 999)), SHORT(qBound(1, rows, 999))};

	return SUCCEEDED(::ResizePseudoConsole(sessionPtr->hPC, size));
#else
	if (sessionPtr->ptyMasterFd < 0){
		return false;
	}
	struct winsize windowSize{};
	windowSize.ws_col = static_cast<unsigned short>(qBound(1, columns, 999));
	windowSize.ws_row = static_cast<unsigned short>(qBound(1, rows, 999));

	// The kernel sends SIGWINCH to the foreground process group automatically.
	return ::ioctl(sessionPtr->ptyMasterFd, TIOCSWINSZ, &windowSize) == 0;
#endif
}


bool CTerminalSessionManagerComp::ResolveShellProgram(ShellType shellType, QString& program, QStringList& arguments) const
{
	arguments.clear();

#if defined(Q_OS_WIN)
	switch (shellType){
	case ST_CMD:
		// ConPTY presents a real console, so cmd.exe needs no special arguments.
		program = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
		break;
	case ST_POWERSHELL:
		program = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
		if (!program.isEmpty()){
			arguments << QStringLiteral("-NoLogo") << QStringLiteral("-NoProfile");
		}
		break;
	default:
		program.clear();
		break;
	}
#else
	switch (shellType){
	case ST_BASH:
		program = QStandardPaths::findExecutable(QStringLiteral("bash"));
		if (program.isEmpty() && QFileInfo::exists(QStringLiteral("/bin/bash"))){
			program = QStringLiteral("/bin/bash");
		}
		// Interactive mode keeps the prompt and reads commands from standard input.
		if (!program.isEmpty()){
			arguments << QStringLiteral("-i");
		}
		break;
	case ST_SH:
		program = QStandardPaths::findExecutable(QStringLiteral("sh"));
		if (program.isEmpty() && QFileInfo::exists(QStringLiteral("/bin/sh"))){
			program = QStringLiteral("/bin/sh");
		}
		// Interactive mode keeps the prompt and reads commands from standard input.
		if (!program.isEmpty()){
			arguments << QStringLiteral("-i");
		}
		break;
	default:
		program.clear();
		break;
	}
#endif

	return !program.isEmpty();
}


void CTerminalSessionManagerComp::AppendChunk(
			const QByteArray& sessionId,
			Session& session,
			StreamType stream,
			const QString& data,
			bool updateActivity)
{
	OutputChunk chunk;
	chunk.sequence = session.nextSequence++;
	chunk.stream = stream;
	chunk.data = data;

	session.chunks.append(chunk);
	session.bufferedBytes += data.toUtf8().size();
	if (updateActivity){
		session.lastActivity = QDateTime::currentDateTimeUtc();
		session.idleWarningSent = false;
	}

	// Enforce the per-session output buffer bound by discarding the oldest chunks.
	while (session.bufferedBytes > MaxSessionBufferBytes && session.chunks.count() > 1){
		const OutputChunk& oldest = session.chunks.first();
		session.bufferedBytes -= oldest.data.toUtf8().size();
		session.firstSequence = oldest.sequence + 1;
		session.chunks.removeFirst();
	}

	// Wake GraphQL publishers (CTerminalOutputPublisherComp). Recursive mutex lets
	// observers call GetOutput from the same thread without deadlocking.
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
	changeSet.SetChangeInfo(CN_TERMINAL_OUTPUT_CHANGED, QVariant(sessionId));
	istd::CChangeNotifier notifier(this, &changeSet);
}


void CTerminalSessionManagerComp::FinalizeIfDone(const QByteArray& sessionId, Session& session, int exitCode)
{
#if defined(Q_OS_WIN)
	// The reader draining the last output and the exit-code notifier can arrive in
	// either order; only finalize once both have happened.
	if (!session.readerFinished || !session.processExited){
		return;
	}
#endif

	if (session.finished){
		return;
	}

	session.finished = true;
	session.exitCode = exitCode;
	AppendChunk(sessionId, session, STREAM_SYSTEM, QString("Session finished with exit code %1").arg(exitCode), false);
}


} // namespace agentinodata
