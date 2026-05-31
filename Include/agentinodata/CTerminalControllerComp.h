// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/ITerminalController.h>


namespace agentinodata
{


/**
	Default implementation of \ref ITerminalController.

	Each session owns a single shell QProcess started with the privileges of the agent
	service. Output of every session is buffered (bounded) and exposed incrementally so
	that the server UI can poll it. The component never builds a shell command line from
	user input: the requested data is written verbatim to the shell standard input.

	Security relevant bounds (concurrent sessions, buffer size, idle timeout) are enforced
	here, and all running shells are killed when the component is destroyed (orderly agent
	shutdown).

	\ingroup Terminal
*/
class CTerminalControllerComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public ITerminalController
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerComp);
	I_END_COMPONENT;

	// reimplemented (agentinodata::ITerminalController)
	virtual QList<ShellInfo> GetAvailableShells() const override;
	virtual QByteArray OpenSession(ShellType shellType, QString& errorMessage) override;
	virtual bool SendInput(const QByteArray& sessionId, const QString& data) override;
	virtual bool CloseSession(const QByteArray& sessionId) override;
	virtual QList<OutputChunk> GetOutput(
				const QByteArray& sessionId,
				qint64 fromSequence,
				qint64& nextSequence,
				bool& running,
				bool& finished,
				int& exitCode) const override;
	virtual bool SessionExists(const QByteArray& sessionId) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

protected Q_SLOTS:
	void OnReadyReadStandardOutput();
	void OnReadyReadStandardError();
	void OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void OnProcessErrorOccurred(QProcess::ProcessError error);
	void OnIdleTimeout();

private:
	struct Session
	{
		istd::TDelPtr<QProcess> processPtr;
		ShellType shellType = ST_BASH;
		QList<OutputChunk> chunks;
		qint64 firstSequence = 0;
		qint64 nextSequence = 0;
		qint64 bufferedBytes = 0;
		bool finished = false;
		int exitCode = -1;
		QDateTime lastActivity;
	};

	bool ResolveShellProgram(ShellType shellType, QString& program, QStringList& arguments) const;
	void AppendChunk(Session& session, StreamType stream, const QString& data);
	QByteArray FindSessionId(const QProcess* processPtr) const;
	void RemoveSession(const QByteArray& sessionId);
	void EmitOutputChangeSignal(const QByteArray& sessionId);

private:
	/** Maximum number of concurrently open sessions. */
	static const int MaxSessionCount = 16;
	/** Maximum amount of buffered output kept per session, in bytes. */
	static const qint64 MaxSessionBufferBytes = 1024 * 1024;
	/** Maximum length of a single input request, in characters. */
	static const int MaxInputLength = 16 * 1024;
	/** Session is auto-closed after this period of inactivity, in seconds. */
	static const int IdleTimeoutSeconds = 15 * 60;
	/** Interval at which idle sessions are checked, in milliseconds. */
	static const int IdleCheckIntervalMs = 30000;

	QMap<QByteArray, Session*> m_sessionMap;
	QTimer m_idleTimer;
	int m_sessionCounter = 0;
};


} // namespace agentinodata
