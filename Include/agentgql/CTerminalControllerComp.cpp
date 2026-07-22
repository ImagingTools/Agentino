// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CTerminalControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal.h>


namespace agentgql
{


namespace
{


/** Wire enum <-> internal enum. Both are generated from the same set of names. */
sdl::V1_0::agentino::ShellType ToWireShellType(agentinodata::ITerminalController::ShellType shellType)
{
	switch (shellType){
	case agentinodata::ITerminalController::ST_CMD:
		return sdl::V1_0::agentino::ShellType::CMD;
	case agentinodata::ITerminalController::ST_POWERSHELL:
		return sdl::V1_0::agentino::ShellType::POWERSHELL;
	case agentinodata::ITerminalController::ST_SH:
		return sdl::V1_0::agentino::ShellType::SH;
	case agentinodata::ITerminalController::ST_BASH:
	default:
		return sdl::V1_0::agentino::ShellType::BASH;
	}
}


agentinodata::ITerminalController::ShellType FromWireShellType(sdl::V1_0::agentino::ShellType shellType)
{
	switch (shellType){
	case sdl::V1_0::agentino::ShellType::CMD:
		return agentinodata::ITerminalController::ST_CMD;
	case sdl::V1_0::agentino::ShellType::POWERSHELL:
		return agentinodata::ITerminalController::ST_POWERSHELL;
	case sdl::V1_0::agentino::ShellType::SH:
		return agentinodata::ITerminalController::ST_SH;
	case sdl::V1_0::agentino::ShellType::BASH:
	default:
		return agentinodata::ITerminalController::ST_BASH;
	}
}


sdl::V1_0::agentino::TerminalStreamType ToWireStreamType(agentinodata::ITerminalController::StreamType streamType)
{
	switch (streamType){
	case agentinodata::ITerminalController::STREAM_STDERR:
		return sdl::V1_0::agentino::TerminalStreamType::STDERR;
	case agentinodata::ITerminalController::STREAM_SYSTEM:
		return sdl::V1_0::agentino::TerminalStreamType::SYSTEM;
	case agentinodata::ITerminalController::STREAM_STDOUT:
	default:
		return sdl::V1_0::agentino::TerminalStreamType::STDOUT;
	}
}


} // namespace


// protected methods

// reimplemented (sdl::V1_0::agentino::CTerminalGqlHandlerCompBase)

sdl::V1_0::agentino::CShellTypeListPayload CTerminalControllerComp::OnListShellTypes(
			const sdl::V1_0::agentino::CListShellTypesGqlRequest& /*listShellTypesRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CShellTypeListPayload response;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to list shell types. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	QList<sdl::V1_0::agentino::CShellTypeInfo> items;
	const QList<agentinodata::ITerminalController::ShellInfo> shells = m_terminalSessionManagerCompPtr->GetAvailableShells();
	for (const agentinodata::ITerminalController::ShellInfo& shell: shells){
		sdl::V1_0::agentino::CShellTypeInfo item;
		item.type = ToWireShellType(shell.type);
		item.name = shell.name;
		item.available = shell.available;

		items << item;
	}

	response.items.Emplace();
	response.items->FromList(items);

	return response;
}


sdl::V1_0::agentino::CTerminalOutputResponse CTerminalControllerComp::OnGetTerminalOutput(
			const sdl::V1_0::agentino::CGetTerminalOutputGqlRequest& getTerminalOutputRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTerminalOutputResponse response;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to read terminal output. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	const sdl::V1_0::agentino::GetTerminalOutputRequestArguments arguments = getTerminalOutputRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->sessionId.has_value()){
		errorMessage = QStringLiteral("Unable to read terminal output. Error: session ID is missing");

		return response;
	}

	const QByteArray sessionId = *arguments.input->sessionId;
	const qint64 fromSequence = arguments.input->fromSequence.has_value() ? *arguments.input->fromSequence : 0;

	qint64 nextSequence = fromSequence;
	bool running = false;
	int exitCode = -1;

	const QList<agentinodata::ITerminalController::OutputChunk> chunks = m_terminalSessionManagerCompPtr->GetOutput(
				sessionId, fromSequence, nextSequence, running, exitCode);

	response.sessionId = sessionId;
	response.nextSequence = int(nextSequence);
	response.running = running;
	response.exitCode = exitCode;

	QList<sdl::V1_0::agentino::CTerminalOutputChunk> chunkList;
	for (const agentinodata::ITerminalController::OutputChunk& chunk: chunks){
		sdl::V1_0::agentino::CTerminalOutputChunk chunkItem;
		chunkItem.sequence = int(chunk.sequence);
		chunkItem.stream = ToWireStreamType(chunk.stream);
		chunkItem.data = chunk.data;

		chunkList << chunkItem;
	}

	response.chunks.Emplace();
	response.chunks->FromList(chunkList);

	return response;
}


sdl::V1_0::agentino::COpenTerminalSessionResponse CTerminalControllerComp::OnOpenTerminalSession(
			const sdl::V1_0::agentino::COpenTerminalSessionGqlRequest& openTerminalSessionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::COpenTerminalSessionResponse response;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to open terminal session. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	const sdl::V1_0::agentino::OpenTerminalSessionRequestArguments arguments = openTerminalSessionRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->shellType.has_value()){
		errorMessage = QStringLiteral("Unable to open terminal session. Error: shell type is missing");

		return response;
	}

	const QByteArray sessionId = m_terminalSessionManagerCompPtr->OpenSession(
				FromWireShellType(*arguments.input->shellType), errorMessage);

	response.sessionId = sessionId;
	response.shellType = *arguments.input->shellType;
	response.started = !sessionId.isEmpty();

	return response;
}


sdl::V1_0::agentino::CSendTerminalInputResponse CTerminalControllerComp::OnSendTerminalInput(
			const sdl::V1_0::agentino::CSendTerminalInputGqlRequest& sendTerminalInputRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSendTerminalInputResponse response;
	response.accepted = false;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to send terminal input. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	const sdl::V1_0::agentino::SendTerminalInputRequestArguments arguments = sendTerminalInputRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->sessionId.has_value()){
		errorMessage = QStringLiteral("Unable to send terminal input. Error: session ID is missing");

		return response;
	}

	const QString data = arguments.input->data.has_value() ? *arguments.input->data : QString();

	response.accepted = m_terminalSessionManagerCompPtr->SendInput(*arguments.input->sessionId, data);

	return response;
}


sdl::V1_0::agentino::CInterruptTerminalSessionResponse CTerminalControllerComp::OnInterruptTerminalSession(
			const sdl::V1_0::agentino::CInterruptTerminalSessionGqlRequest& interruptTerminalSessionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CInterruptTerminalSessionResponse response;
	response.accepted = false;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to interrupt terminal session. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	const sdl::V1_0::agentino::InterruptTerminalSessionRequestArguments arguments =
				interruptTerminalSessionRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->sessionId.has_value()){
		errorMessage = QStringLiteral("Unable to interrupt terminal session. Error: session ID is missing");

		return response;
	}

	response.accepted = m_terminalSessionManagerCompPtr->InterruptSession(*arguments.input->sessionId);

	return response;
}


sdl::V1_0::agentino::CCloseTerminalSessionResponse CTerminalControllerComp::OnCloseTerminalSession(
			const sdl::V1_0::agentino::CCloseTerminalSessionGqlRequest& closeTerminalSessionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CCloseTerminalSessionResponse response;
	response.closed = false;

	if (!m_terminalSessionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to close terminal session. Error: attribute 'TerminalSessionManager' was not set");

		return response;
	}

	const sdl::V1_0::agentino::CloseTerminalSessionRequestArguments arguments = closeTerminalSessionRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->sessionId.has_value()){
		errorMessage = QStringLiteral("Unable to close terminal session. Error: session ID is missing");

		return response;
	}

	response.closed = m_terminalSessionManagerCompPtr->CloseSession(*arguments.input->sessionId);

	return response;
}


} // namespace agentgql
