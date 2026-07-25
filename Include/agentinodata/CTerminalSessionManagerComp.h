// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QStringConverter>
#include <QtCore/QTimer>

// moc does not see Q_OS_WIN (it only expands the macros this project's CMake passes
// explicitly - _WIN32 is not among them, only WIN32/_WIN64 - so a class guarded by
// #if defined(Q_OS_WIN) alone would silently vanish from the generated moc file with
// no error, just missing metaobject symbols at link time). WIN32/_WIN64 are always
// defined by CMake's own Windows generators, independent of the project's own defines,
// so this local macro is visible to moc as well as the real compiler.
#if defined(Q_OS_WIN) || defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#	define AGENTINO_TERMINAL_WIN 1
#endif

#if defined(AGENTINO_TERMINAL_WIN)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
	// ConPTY (CreatePseudoConsole etc.) is only declared for Windows 10+; match what
	// Qt's own qt_windows.h would set, in case this header is included before any Qt
	// header that pulls it in first.
#	ifndef WINVER
#		define WINVER 0x0A00
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0A00
#	endif
#	include <windows.h>
#	include <QtCore/QThread>
#	include <QtCore/QWinEventNotifier>
#else
#	include <QtCore/QSocketNotifier>
#endif

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/ITerminalController.h>


namespace agentinodata
{


#if defined(AGENTINO_TERMINAL_WIN)

/**
	Blocking reader for one session's ConPTY output pipe, run on its own thread since
	Win32 anonymous pipes have no event-loop-friendly (overlapped) read primitive that
	Qt can wait on directly. Owned/started/stopped by \ref CTerminalSessionManagerComp;
	the pipe handle is closed by the owner to unblock \c ReadFile and end \c run().

	\ingroup Terminal
*/
class CTerminalPtyReaderThread: public QThread
{
	Q_OBJECT
public:
	explicit CTerminalPtyReaderThread(HANDLE pipeReadHandle, QObject* parentPtr = nullptr);

Q_SIGNALS:
	void DataRead(const QByteArray& data);
	void ReaderFinished();

protected:
	// reimplemented (QThread)
	virtual void run() override;

private:
	HANDLE m_pipeReadHandle;
};

#endif // Q_OS_WIN


/**
	Default implementation of \ref ITerminalController.

	Owns the real terminal sessions on the agent host: each session is a single shell
	process attached to a real pseudo-terminal (ConPTY on Windows, \c openpty on
	Linux/macOS) - not a plain pipe - so full-screen/curses programs (vim, mc, top) and
	ANSI color output work as they would in a native terminal. Output of every session is
	buffered (bounded) and exposed incrementally; new chunks raise
	\ref ITerminalController::CN_TERMINAL_OUTPUT_CHANGED so GraphQL publishers can push
	to subscribers (GetOutput remains for catch-up). The component never builds a shell
	command line from user input: the requested data is written verbatim to the pty.

	Named "session manager" (not "controller") to keep it distinct from the GQL-facing
	\c agentgql::CTerminalControllerComp, which is a thin per-request resolver that
	delegates every call here - same split as \c CServiceSupervisorComp (data layer) vs
	\c agentgql::CServiceControllerComp (GQL layer). This component must live outside the
	GQL handler's request-scoped composite (see \c TerminalController.acc): the handler
	tree is recreated per request, so session state (open shells, buffered output) would
	not survive between an OpenSession call and the next poll if it lived there.

	\note Thread affinity: the agent answers GraphQL requests on worker threads, but the
	native process/pty handles may only be used from the thread that created them. Every
	operation that touches them is therefore executed on this component's own thread (the
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
	virtual bool SendRawInput(const QByteArray& sessionId, const QString& data) override;
	virtual bool CloseSession(const QByteArray& sessionId) override;
	virtual bool InterruptSession(const QByteArray& sessionId) override;
	virtual bool ResizeSession(const QByteArray& sessionId, int columns, int rows) override;
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
	void OnIdleTimeout();

private:
	struct Session
	{
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
		// A real pty merges stdout+stderr onto one stream (both are the same fd/pipe),
		// so only one decoder is needed; kept on the session so a multi-byte character
		// split across two reads still decodes correctly.
		QStringDecoder ptyDecoder = QStringDecoder(QStringDecoder::Utf8);
		QString pendingOutput;
		bool outputFlushScheduled = false;

#if defined(AGENTINO_TERMINAL_WIN)
		HPCON hPC = nullptr;
		PROCESS_INFORMATION processInfo{};
		HANDLE pipeInWrite = nullptr;   // agent -> child stdin
		HANDLE pipeOutRead = nullptr;   // child stdout/stderr -> agent
		istd::TDelPtr<CTerminalPtyReaderThread> readerThreadPtr;
		istd::TDelPtr<QWinEventNotifier> exitNotifierPtr;
		// Both must be true before the session is finalized as finished - the reader
		// draining the last output and the exit-code notifier can arrive in either order.
		bool readerFinished = false;
		bool processExited = false;
#else
		istd::TDelPtr<QProcess> processPtr;
		int ptyMasterFd = -1;
		istd::TDelPtr<QSocketNotifier> readNotifierPtr;
#endif
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
	bool SendRawInputOnOwnThread(const QByteArray& sessionId, const QString& data);
	bool InterruptSessionOnOwnThread(const QByteArray& sessionId);
	bool ResizeSessionOnOwnThread(const QByteArray& sessionId, int columns, int rows);
	/**
		Write \p bytes to the session's pty verbatim - no newline massaging. Used by both
		\ref SendInputOnOwnThread (which appends a newline first) and interrupt (a single
		raw 0x03 byte must reach the pty with nothing appended after it).
	*/
	bool WriteRawOnOwnThread(Session& session, const QByteArray& bytes);
	void AppendChunk(
				const QByteArray& sessionId,
				Session& session,
				StreamType stream,
				const QString& data,
				bool updateActivity = true);
			void QueueOutput(const QByteArray& sessionId, Session& session, const QString& data);
			void FlushPendingOutput(const QByteArray& sessionId, Session& session);
	void RemoveSession(const QByteArray& sessionId);
	// Marks the session finished/appends the SYSTEM chunk once every platform-specific
	// shutdown signal for it has arrived (see the two bool flags on Session, Windows only -
	// Unix has a single QProcess::finished signal and needs no such join).
	void FinalizeIfDone(const QByteArray& sessionId, Session& session, int exitCode);

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
	/** Window for combining adjacent native pty reads into one output notification. */
	static const int OutputFlushIntervalMs = 10;
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
