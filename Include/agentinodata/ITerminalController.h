// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{


/**
	Interface to control interactive remote terminal (shell) sessions on the agent machine.

	A session wraps a single shell process (cmd/PowerShell on Windows, bash/sh on Unix).
	The operator opens a session, sends input lines to the shell standard input and reads
	the produced output incrementally.

	\ingroup Terminal
*/
class ITerminalController: virtual public istd::IChangeable
{
public:
	/**
		Supported shell types.
		\note The numeric values must stay in sync with the 'ShellType' enum defined in Terminal.sdl.
	*/
	enum ShellType
	{
		ST_CMD = 0,
		ST_POWERSHELL = 1,
		ST_BASH = 2,
		ST_SH = 3
	};

	/**
		Type of an output stream a chunk was produced on.
		\note The numeric values must stay in sync with the 'TerminalStreamType' enum defined in Terminal.sdl.
	*/
	enum StreamType
	{
		STREAM_STDOUT = 0,
		STREAM_STDERR = 1,
		STREAM_SYSTEM = 2
	};

	/**
		A single incremental chunk of terminal output.
	*/
	struct OutputChunk
	{
		qint64 sequence = 0;
		StreamType stream = STREAM_STDOUT;
		QString data;
	};

	/**
		Description of a shell type and whether it is available on the agent machine.
	*/
	struct ShellInfo
	{
		ShellType type = ST_BASH;
		QString name;
		bool available = false;
	};

	/**
		Change notification emitted when new output is available for a session.
	*/
	static const QByteArray CN_TERMINAL_OUTPUT_CHANGED;

	/**
		Get the list of shell types that can be launched on the agent machine.
	*/
	virtual QList<ShellInfo> GetAvailableShells() const = 0;

	/**
		Open a new terminal session running the requested shell.
		\param shellType Shell to launch.
		\param errorMessage Filled with a human readable error if the session could not be opened.
		\return The new session identifier or an empty value on failure.
	*/
	virtual QByteArray OpenSession(ShellType shellType, QString& errorMessage) = 0;

	/**
		Send a chunk of data to the standard input of the shell of the given session.
		A trailing new line is appended automatically when missing so that typed commands are executed.
	*/
	virtual bool SendInput(const QByteArray& sessionId, const QString& data) = 0;

	/**
		Terminate the shell of the given session and release all associated resources.
	*/
	virtual bool CloseSession(const QByteArray& sessionId) = 0;

	/**
		Read output produced by the session since the given sequence number.
		\param sessionId Session to read from.
		\param fromSequence Sequence number of the first chunk the caller has not seen yet (0 to read from start).
		\param nextSequence Filled with the sequence number the caller should request next time.
		\param running Filled with true while the shell process is still alive.
		\param finished Filled with true once the shell process has exited.
		\param exitCode Filled with the process exit code when finished.
		\return The list of new output chunks, empty when nothing new is available.
	*/
	virtual QList<OutputChunk> GetOutput(
				const QByteArray& sessionId,
				qint64 fromSequence,
				qint64& nextSequence,
				bool& running,
				bool& finished,
				int& exitCode) const = 0;

	/**
		Check whether a session with the given identifier currently exists.
	*/
	virtual bool SessionExists(const QByteArray& sessionId) const = 0;
};


} // namespace agentinodata
