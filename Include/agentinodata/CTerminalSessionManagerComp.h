// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QStringConverter>
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

	Owns the real terminal sessions on the agent host: each session is a single shell
	QProcess started with the privileges of the agent service. Output of every session is
	buffered (bounded) and exposed incrementally; new chunks raise
	\ref ITerminalController::CN_TERMINAL_OUTPUT_CHANGED so GraphQL publishers can push
	to subscribers (GetOutput remains for catch-up). The component never builds a shell
	command line from user input: the requested data is written verbatim to the shell
	standard input.

	Named "session manager" (not "controller") to keep it distinct from the GQL-facing
	\c agentgql::CTerminalControllerComp, which is a thin per-request resolver that
	delegates every call here - same split as \c CServiceSupervisorComp (data layer) vs
	\c agentgql::CServiceControllerComp (GQL layer). This component must live outside the
	GQL handler's request-scoped composite (see \c TerminalController.acc): the handler
	tree is recreated per request, so session state (open shells, buffered output) would
	not survive between an OpenSession call and the next poll if it lived there.

	\note Thread affinity: the agent answers GraphQL requests on worker threads, but a
	QProcess may only be used from the thread that created it. Every operation that
	touches a process is therefore executed on this component's own thread (the
	application thread) - see \ref RunOnComponentThread - while the session book keeping
	is protected by \ref m_mutex so that read-only polling never has to leave the worker.

	Security relevant bounds (concurrent sessions, buffer size, idle timeout) are enforced
	here, and all running shells are killed when the component is destroyed (orderly agent
	shutdown).

	\ingroup Terminal
*/
class CTerminalSessionManagerComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public ITerminalController
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalSessionManagerComp);
		I_REGISTER_INTERFACE(agentinodata::ITerminalController)
	I_END_COMPONENT;

	// reimplemented (agentinodata::ITerminalController)
	virtual QList<ShellInfo> GetAvailableShells() const override;
	virtual QByteArray OpenSession(ShellType shellType, QString& errorMessage) override;
	virtual bool SendInput(const QByteArray& sessionId, const QString& data) override;
	virtual bool CloseSession(const QByteArray& sessionId) override;
	virtual bool InterruptSession(const QByteArray& sessionId) override;
	virtual QList<OutputChunk> GetOutput(
				const QByteArray& sessionId,
				qint64 fromSequence,
				qint64& nextSequence,
				bool& running,
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
		// True after the ~60s idle warning was pushed; reset on real user/process activity.
		bool idleWarningSent = false;
		// Kept per stream so that a multi byte character split over two reads is decoded
		// correctly instead of being turned into replacement characters.
		QStringDecoder stdOutDecoder = QStringDecoder(QStringDecoder::Utf8);
		QStringDecoder stdErrDecoder = QStringDecoder(QStringDecoder::Utf8);
	};

	/**
		Execute \p func on this component's thread, blocking the calling worker thread
		until it is done. Called on the own thread the functor runs directly.
	*/
	template <class Func>
	void RunOnComponentThread(Func func);

	bool ResolveShellProgram(ShellType shellType, QString& program, QStringList& arguments) const;
	QByteArray OpenSessionOnOwnThread(ShellType shellType, QString& errorMessage);
	bool SendInputOnOwnThread(const QByteArray& sessionId, const QString& data);
	bool InterruptSessionOnOwnThread(const QByteArray& sessionId);
	void AppendChunk(
				const QByteArray& sessionId,
				Session& session,
				StreamType stream,
				const QString& data,
				bool updateActivity = true);
	QByteArray FindSessionId(const QProcess* processPtr) const;
	void RemoveSession(const QByteArray& sessionId);

private:
	/** Maximum number of concurrently open sessions. */
	static const int MaxSessionCount = 16;
	/** Maximum amount of buffered output kept per session, in bytes. */
	static const qint64 MaxSessionBufferBytes = 1024 * 1024;
	/** Maximum length of a single input request, in characters. */
	static const int MaxInputLength = 16 * 1024;
	/** Session is auto-closed after this period of inactivity, in seconds. */
	static const int IdleTimeoutSeconds = 15 * 60;
	/** Lead time before idle close when a SYSTEM warning is pushed, in seconds. */
	static const int IdleWarningLeadSeconds = 60;
	/** Interval at which idle sessions are checked, in milliseconds. */
	static const int IdleCheckIntervalMs = 30000;
	/** Exit code written when a session is closed by the idle timer (not a process exit). */
	static const int IdleCloseExitCode = -2;

	QMap<QByteArray, Session*> m_sessionMap;
	QTimer m_idleTimer;
	int m_sessionCounter = 0;
	// Recursive: QProcess::waitForStarted() can deliver errorOccurred() synchronously
	// into a slot that locks again while the opening call still holds the lock.
	mutable QRecursiveMutex m_mutex;
};


} // namespace agentinodata
