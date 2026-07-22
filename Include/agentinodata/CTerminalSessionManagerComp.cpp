// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CTerminalSessionManagerComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>
#include <QtCore/QThread>
#include <QtCore/QUuid>

// ACF includes
#include <istd/CChangeNotifier.h>

#if defined(Q_OS_UNIX)
#include <signal.h>
#include <sys/types.h>
#endif


namespace agentinodata
{


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
	// GetOutput — the publisher and catch-up reads must not keep a session alive.

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

void CTerminalSessionManagerComp::OnReadyReadStandardOutput()
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QMutexLocker locker(&m_mutex);

	const QByteArray sessionId = FindSessionId(processPtr);
	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		return;
	}

	const QString output = sessionPtr->stdOutDecoder.decode(processPtr->readAllStandardOutput());
	if (!output.isEmpty()){
		AppendChunk(sessionId, *sessionPtr, STREAM_STDOUT, output);
	}
}


void CTerminalSessionManagerComp::OnReadyReadStandardError()
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QMutexLocker locker(&m_mutex);

	const QByteArray sessionId = FindSessionId(processPtr);
	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		return;
	}

	const QString output = sessionPtr->stdErrDecoder.decode(processPtr->readAllStandardError());
	if (!output.isEmpty()){
		AppendChunk(sessionId, *sessionPtr, STREAM_STDERR, output);
	}
}


void CTerminalSessionManagerComp::OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QMutexLocker locker(&m_mutex);

	const QByteArray sessionId = FindSessionId(processPtr);
	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		return;
	}

	// Drain any remaining buffered output before marking the session finished.
	const QString remainingOut = sessionPtr->stdOutDecoder.decode(processPtr->readAllStandardOutput());
	if (!remainingOut.isEmpty()){
		AppendChunk(sessionId, *sessionPtr, STREAM_STDOUT, remainingOut);
	}
	const QString remainingErr = sessionPtr->stdErrDecoder.decode(processPtr->readAllStandardError());
	if (!remainingErr.isEmpty()){
		AppendChunk(sessionId, *sessionPtr, STREAM_STDERR, remainingErr);
	}

	sessionPtr->finished = true;
	sessionPtr->exitCode = exitCode;

	const QString statusText = (exitStatus == QProcess::NormalExit)
				? QString("Session finished with exit code %1").arg(exitCode)
				: QString("Session terminated abnormally");
	AppendChunk(sessionId, *sessionPtr, STREAM_SYSTEM, statusText);
}


void CTerminalSessionManagerComp::OnProcessErrorOccurred(QProcess::ProcessError error)
{
	Q_UNUSED(error);

	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QMutexLocker locker(&m_mutex);

	const QByteArray sessionId = FindSessionId(processPtr);
	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr){
		return;
	}

	AppendChunk(sessionId, *sessionPtr, STREAM_SYSTEM, QString("Process error: %1").arg(processPtr->errorString()));
}


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

	// Worker path: the shell processes belong to this component's thread, which is
	// inside the application event loop while workers serve requests, so blocking
	// here cannot dead-lock (same reasoning as imtcom::CRequestSender).
	QMetaObject::invokeMethod(this, func, Qt::BlockingQueuedConnection);
}


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

	istd::TDelPtr<Session> sessionPtr(new Session);
	sessionPtr->shellType = shellType;
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->processPtr.SetPtr(new QProcess(this));

	QProcess* processPtr = sessionPtr->processPtr.GetPtr();
	processPtr->setProcessChannelMode(QProcess::SeparateChannels);
	processPtr->setProgram(program);
	processPtr->setArguments(arguments);
	processPtr->setWorkingDirectory(QDir::homePath());

	// The output is rendered as plain text: ask the shell not to emit terminal control
	// sequences that would otherwise show up as unreadable escape codes.
	QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
	processEnvironment.insert(QStringLiteral("TERM"), QStringLiteral("dumb"));
	processPtr->setProcessEnvironment(processEnvironment);

	connect(processPtr, &QProcess::readyReadStandardOutput, this, &CTerminalSessionManagerComp::OnReadyReadStandardOutput);
	connect(processPtr, &QProcess::readyReadStandardError, this, &CTerminalSessionManagerComp::OnReadyReadStandardError);
	connect(processPtr, &QProcess::finished, this, &CTerminalSessionManagerComp::OnProcessFinished);
	connect(processPtr, &QProcess::errorOccurred, this, &CTerminalSessionManagerComp::OnProcessErrorOccurred);

	processPtr->start(QIODevice::ReadWrite);
	if (!processPtr->waitForStarted(5000)){
		errorMessage = QString("Unable to start shell '%1': %2").arg(program, processPtr->errorString());
		SendErrorMessage(0, errorMessage, "CTerminalSessionManagerComp");

		return QByteArray();
	}

	const QByteArray sessionId = QByteArrayLiteral("term-")
				+ QByteArray::number(++m_sessionCounter)
				+ '-'
				+ QUuid::createUuid().toByteArray(QUuid::Id128);

	Session* rawSessionPtr = sessionPtr.PopPtr();
	m_sessionMap.insert(sessionId, rawSessionPtr);

	AppendChunk(sessionId, *rawSessionPtr, STREAM_SYSTEM, QString("Session started (%1)").arg(program));

	SendInfoMessage(0, QString("Terminal session '%1' started (%2)").arg(QString(sessionId), program), "CTerminalSessionManagerComp");

	return sessionId;
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

	if (sessionPtr->finished || !sessionPtr->processPtr.IsValid()){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' is no longer running").arg(QString(sessionId)), "CTerminalSessionManagerComp");

		return false;
	}

	QProcess* processPtr = sessionPtr->processPtr.GetPtr();
	if (processPtr->state() != QProcess::Running){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' is no longer running").arg(QString(sessionId)), "CTerminalSessionManagerComp");

		return false;
	}

	// The user input is written verbatim to the shell standard input. A trailing new line is
	// added only when missing so the typed command is actually executed by the shell.
	QString payload = data;
	if (!payload.endsWith('\n')){
		payload.append('\n');
	}

	const qint64 written = processPtr->write(payload.toUtf8());
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->idleWarningSent = false;

	return written >= 0;
}


bool CTerminalSessionManagerComp::InterruptSessionOnOwnThread(const QByteArray& sessionId)
{
	QMutexLocker locker(&m_mutex);

	Session* sessionPtr = m_sessionMap.value(sessionId, nullptr);
	if (sessionPtr == nullptr || sessionPtr->finished || !sessionPtr->processPtr.IsValid()){
		return false;
	}

	QProcess* processPtr = sessionPtr->processPtr.GetPtr();
	if (processPtr->state() != QProcess::Running){
		return false;
	}

	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();
	sessionPtr->idleWarningSent = false;

#if defined(Q_OS_UNIX)
	// Best-effort SIGINT to the shell process group (interactive shells typically
	// forward it to the foreground job). Does not destroy the shell itself.
	const qint64 pid = processPtr->processId();
	if (pid > 0){
		::kill(static_cast<pid_t>(pid), SIGINT);
	}
#endif

	// Also inject the Ctrl+C character into stdin — helps cmd/PowerShell and some
	// interactive programs that do not listen for signals.
	const char ctrlC = 0x03;
	processPtr->write(&ctrlC, 1);

	AppendChunk(sessionId, *sessionPtr, STREAM_SYSTEM, QStringLiteral("^C"), true);

	return true;
}


bool CTerminalSessionManagerComp::ResolveShellProgram(ShellType shellType, QString& program, QStringList& arguments) const
{
	arguments.clear();

#if defined(Q_OS_WIN)
	switch (shellType){
	case ST_CMD:
		program = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
		if (!program.isEmpty()){
			// /Q: no command echo, /K: stay interactive and read commands from stdin.
			// The code page is switched to UTF-8 so the output decodes without mojibake.
			arguments << QStringLiteral("/Q") << QStringLiteral("/K") << QStringLiteral("chcp 65001>nul");
		}
		break;
	case ST_POWERSHELL:
		program = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
		if (!program.isEmpty()){
			// -NoExit keeps the interpreter reading commands from stdin after the
			// encoding preamble. Setting the console encoding fails when the process
			// has no console attached, which is not an error worth reporting.
			arguments << QStringLiteral("-NoLogo")
					  << QStringLiteral("-NoProfile")
					  << QStringLiteral("-NoExit")
					  << QStringLiteral("-Command")
					  << QStringLiteral("$OutputEncoding = [System.Text.Encoding]::UTF8;"
										" try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }");
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


QByteArray CTerminalSessionManagerComp::FindSessionId(const QProcess* processPtr) const
{
	for (auto it = m_sessionMap.constBegin(); it != m_sessionMap.constEnd(); ++it){
		if (it.value() != nullptr && it.value()->processPtr.GetPtr() == processPtr){
			return it.key();
		}
	}

	return QByteArray();
}


void CTerminalSessionManagerComp::RemoveSession(const QByteArray& sessionId)
{
	Session* sessionPtr = m_sessionMap.take(sessionId);
	if (sessionPtr == nullptr){
		return;
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


} // namespace agentinodata
