// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CTerminalControllerComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/IChangeable.h>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::ITerminalController)

QList<ITerminalController::ShellInfo> CTerminalControllerComp::GetAvailableShells() const
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


QByteArray CTerminalControllerComp::OpenSession(ShellType shellType, QString& errorMessage)
{
	if (m_sessionMap.count() >= MaxSessionCount){
		errorMessage = QString("Unable to open terminal session: maximum number of sessions (%1) reached").arg(MaxSessionCount);
		SendErrorMessage(0, errorMessage, "CTerminalControllerComp");

		return QByteArray();
	}

	QString program;
	QStringList arguments;
	if (!ResolveShellProgram(shellType, program, arguments)){
		errorMessage = QString("Unable to open terminal session: requested shell is not available on this machine");
		SendErrorMessage(0, errorMessage, "CTerminalControllerComp");

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

	connect(processPtr, SIGNAL(readyReadStandardOutput()), this, SLOT(OnReadyReadStandardOutput()));
	connect(processPtr, SIGNAL(readyReadStandardError()), this, SLOT(OnReadyReadStandardError()));
	connect(processPtr, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CTerminalControllerComp::OnProcessFinished);
	connect(processPtr, &QProcess::errorOccurred, this, &CTerminalControllerComp::OnProcessErrorOccurred);

	processPtr->start(QIODevice::ReadWrite);
	if (!processPtr->waitForStarted(5000)){
		errorMessage = QString("Unable to start shell '%1': %2").arg(program, processPtr->errorString());
		SendErrorMessage(0, errorMessage, "CTerminalControllerComp");

		return QByteArray();
	}

	QByteArray sessionId = QByteArrayLiteral("term-")
				+ QByteArray::number(++m_sessionCounter)
				+ '-'
				+ QUuid::createUuid().toByteArray(QUuid::Id128);

	Session* rawSessionPtr = sessionPtr.PopPtr();
	m_sessionMap.insert(sessionId, rawSessionPtr);

	AppendChunk(*rawSessionPtr, STREAM_SYSTEM, QString("Session started (%1)").arg(program));

	SendInfoMessage(0, QString("Terminal session '%1' started (%2)").arg(QString(sessionId), program), "CTerminalControllerComp");

	return sessionId;
}


bool CTerminalControllerComp::SendInput(const QByteArray& sessionId, const QString& data)
{
	if (!m_sessionMap.contains(sessionId)){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' does not exist").arg(QString(sessionId)), "CTerminalControllerComp");

		return false;
	}

	if (data.length() > MaxInputLength){
		SendErrorMessage(0, QString("Unable to send input: request exceeds the maximum allowed length (%1)").arg(MaxInputLength), "CTerminalControllerComp");

		return false;
	}

	Session* sessionPtr = m_sessionMap.value(sessionId);
	Q_ASSERT(sessionPtr != nullptr);

	if (sessionPtr->finished || !sessionPtr->processPtr.IsValid()){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' is no longer running").arg(QString(sessionId)), "CTerminalControllerComp");

		return false;
	}

	QProcess* processPtr = sessionPtr->processPtr.GetPtr();
	if (processPtr->state() != QProcess::Running){
		SendErrorMessage(0, QString("Unable to send input: terminal session '%1' is no longer running").arg(QString(sessionId)), "CTerminalControllerComp");

		return false;
	}

	// The user input is written verbatim to the shell standard input. A trailing new line is
	// added only when missing so the typed command is actually executed by the shell.
	QString payload = data;
	if (!payload.endsWith('\n')){
		payload.append('\n');
	}

	qint64 written = processPtr->write(payload.toUtf8());
	sessionPtr->lastActivity = QDateTime::currentDateTimeUtc();

	return written >= 0;
}


bool CTerminalControllerComp::CloseSession(const QByteArray& sessionId)
{
	if (!m_sessionMap.contains(sessionId)){
		return false;
	}

	RemoveSession(sessionId);

	SendInfoMessage(0, QString("Terminal session '%1' closed").arg(QString(sessionId)), "CTerminalControllerComp");

	return true;
}


QList<ITerminalController::OutputChunk> CTerminalControllerComp::GetOutput(
			const QByteArray& sessionId,
			qint64 fromSequence,
			qint64& nextSequence,
			bool& running,
			bool& finished,
			int& exitCode) const
{
	QList<OutputChunk> retVal;

	nextSequence = fromSequence;
	running = false;
	finished = true;
	exitCode = -1;

	if (!m_sessionMap.contains(sessionId)){
		return retVal;
	}

	const Session* sessionPtr = m_sessionMap.value(sessionId);
	Q_ASSERT(sessionPtr != nullptr);

	finished = sessionPtr->finished;
	running = !sessionPtr->finished;
	exitCode = sessionPtr->exitCode;

	for (const OutputChunk& chunk: sessionPtr->chunks){
		if (chunk.sequence >= fromSequence){
			retVal.append(chunk);
		}
	}

	nextSequence = sessionPtr->nextSequence;

	return retVal;
}


bool CTerminalControllerComp::SessionExists(const QByteArray& sessionId) const
{
	return m_sessionMap.contains(sessionId);
}


// reimplemented (icomp::CComponentBase)

void CTerminalControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	m_idleTimer.setInterval(30000);
	connect(&m_idleTimer, &QTimer::timeout, this, &CTerminalControllerComp::OnIdleTimeout);
	m_idleTimer.start();
}


void CTerminalControllerComp::OnComponentDestroyed()
{
	m_idleTimer.stop();

	const QList<QByteArray> sessionIds = m_sessionMap.keys();
	for (const QByteArray& sessionId: sessionIds){
		RemoveSession(sessionId);
	}

	BaseClass::OnComponentDestroyed();
}


// protected slots

void CTerminalControllerComp::OnReadyReadStandardOutput()
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QByteArray sessionId = FindSessionId(processPtr);
	if (sessionId.isEmpty()){
		return;
	}

	Session* sessionPtr = m_sessionMap.value(sessionId);
	const QString output = QString::fromUtf8(processPtr->readAllStandardOutput());
	if (!output.isEmpty()){
		AppendChunk(*sessionPtr, STREAM_STDOUT, output);
		EmitOutputChangeSignal(sessionId);
	}
}


void CTerminalControllerComp::OnReadyReadStandardError()
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QByteArray sessionId = FindSessionId(processPtr);
	if (sessionId.isEmpty()){
		return;
	}

	Session* sessionPtr = m_sessionMap.value(sessionId);
	const QString output = QString::fromUtf8(processPtr->readAllStandardError());
	if (!output.isEmpty()){
		AppendChunk(*sessionPtr, STREAM_STDERR, output);
		EmitOutputChangeSignal(sessionId);
	}
}


void CTerminalControllerComp::OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QByteArray sessionId = FindSessionId(processPtr);
	if (sessionId.isEmpty()){
		return;
	}

	Session* sessionPtr = m_sessionMap.value(sessionId);

	// Drain any remaining buffered output before marking the session finished.
	const QString remainingOut = QString::fromUtf8(processPtr->readAllStandardOutput());
	if (!remainingOut.isEmpty()){
		AppendChunk(*sessionPtr, STREAM_STDOUT, remainingOut);
	}
	const QString remainingErr = QString::fromUtf8(processPtr->readAllStandardError());
	if (!remainingErr.isEmpty()){
		AppendChunk(*sessionPtr, STREAM_STDERR, remainingErr);
	}

	sessionPtr->finished = true;
	sessionPtr->exitCode = exitCode;

	const QString statusText = (exitStatus == QProcess::NormalExit)
				? QString("Session finished with exit code %1").arg(exitCode)
				: QString("Session terminated abnormally");
	AppendChunk(*sessionPtr, STREAM_SYSTEM, statusText);

	EmitOutputChangeSignal(sessionId);
}


void CTerminalControllerComp::OnProcessErrorOccurred(QProcess::ProcessError error)
{
	QProcess* processPtr = qobject_cast<QProcess*>(sender());
	if (processPtr == nullptr){
		return;
	}

	QByteArray sessionId = FindSessionId(processPtr);
	if (sessionId.isEmpty()){
		return;
	}

	Session* sessionPtr = m_sessionMap.value(sessionId);
	AppendChunk(*sessionPtr, STREAM_SYSTEM, QString("Process error: %1").arg(processPtr->errorString()));

	Q_UNUSED(error);

	EmitOutputChangeSignal(sessionId);
}


void CTerminalControllerComp::OnIdleTimeout()
{
	const QDateTime now = QDateTime::currentDateTimeUtc();

	const QList<QByteArray> sessionIds = m_sessionMap.keys();
	for (const QByteArray& sessionId: sessionIds){
		const Session* sessionPtr = m_sessionMap.value(sessionId);
		if (sessionPtr == nullptr){
			continue;
		}

		if (sessionPtr->lastActivity.secsTo(now) >= IdleTimeoutSeconds){
			SendInfoMessage(0, QString("Terminal session '%1' closed after being idle").arg(QString(sessionId)), "CTerminalControllerComp");
			RemoveSession(sessionId);
		}
	}
}


// private methods

bool CTerminalControllerComp::ResolveShellProgram(ShellType shellType, QString& program, QStringList& arguments) const
{
	arguments.clear();

#if defined(Q_OS_WIN)
	switch (shellType){
	case ST_CMD:
		program = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
		break;
	case ST_POWERSHELL:
		program = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
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


void CTerminalControllerComp::AppendChunk(Session& session, StreamType stream, const QString& data)
{
	OutputChunk chunk;
	chunk.sequence = session.nextSequence++;
	chunk.stream = stream;
	chunk.data = data;

	session.chunks.append(chunk);
	session.bufferedBytes += data.toUtf8().size();
	session.lastActivity = QDateTime::currentDateTimeUtc();

	// Enforce the per-session output buffer bound by discarding the oldest chunks.
	while (session.bufferedBytes > MaxSessionBufferBytes && session.chunks.count() > 1){
		const OutputChunk& oldest = session.chunks.first();
		session.bufferedBytes -= oldest.data.toUtf8().size();
		session.firstSequence = oldest.sequence + 1;
		session.chunks.removeFirst();
	}
}


QByteArray CTerminalControllerComp::FindSessionId(const QProcess* processPtr) const
{
	for (auto it = m_sessionMap.constBegin(); it != m_sessionMap.constEnd(); ++it){
		if (it.value() != nullptr && it.value()->processPtr.GetPtr() == processPtr){
			return it.key();
		}
	}

	return QByteArray();
}


void CTerminalControllerComp::RemoveSession(const QByteArray& sessionId)
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


void CTerminalControllerComp::EmitOutputChangeSignal(const QByteArray& sessionId)
{
	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);

	changeSet.SetChangeInfo(CN_TERMINAL_OUTPUT_CHANGED, sessionId);
	changeSet.SetChangeInfo("sessionid", sessionId);

	istd::CChangeNotifier notifier(this, &changeSet);
}


} // namespace agentinodata
