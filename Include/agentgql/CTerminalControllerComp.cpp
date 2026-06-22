// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CTerminalControllerComp.h>


namespace agentgql
{


// protected methods

// reimplemented (sdl::agentino::Terminal::CGraphQlHandlerCompBase)

sdl::agentino::Terminal::CShellTypeListPayload CTerminalControllerComp::OnListShellTypes(
			const sdl::agentino::Terminal::CListShellTypesGqlRequest& /*listShellTypesRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CShellTypeListPayload response;

	if (!m_terminalControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TerminalController' was not set", "CTerminalControllerComp");

		return response;
	}

	response.Version_1_0.emplace();

	QList<sdl::agentino::Terminal::CShellTypeInfo::V1_0> items;
	const QList<agentinodata::ITerminalController::ShellInfo> shells = m_terminalControllerCompPtr->GetAvailableShells();
	for (const agentinodata::ITerminalController::ShellInfo& shell: shells){
		sdl::agentino::Terminal::CShellTypeInfo::V1_0 item;
		item.type = (sdl::agentino::Terminal::ShellType) shell.type;
		item.name = shell.name;
		item.available = shell.available;

		items << item;
	}

	response.Version_1_0->items.Emplace();
	response.Version_1_0->items->FromList(items);

	return response;
}


sdl::agentino::Terminal::CTerminalOutputResponse CTerminalControllerComp::OnGetTerminalOutput(
			const sdl::agentino::Terminal::CGetTerminalOutputGqlRequest& getTerminalOutputRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CTerminalOutputResponse response;

	if (!m_terminalControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TerminalController' was not set", "CTerminalControllerComp");

		return response;
	}

	sdl::agentino::Terminal::GetTerminalOutputRequestArguments arguments = getTerminalOutputRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CTerminalControllerComp");

		return response;
	}

	if (!arguments.input.Version_1_0->sessionId.has_value()){
		errorMessage = QString("Unable to read terminal output with empty session ID");

		return response;
	}

	QByteArray sessionId = *arguments.input.Version_1_0->sessionId;
	qint64 fromSequence = 0;
	if (arguments.input.Version_1_0->fromSequence.has_value()){
		fromSequence = *arguments.input.Version_1_0->fromSequence;
	}

	qint64 nextSequence = fromSequence;
	bool running = false;
	bool finished = true;
	int exitCode = -1;

	const QList<agentinodata::ITerminalController::OutputChunk> chunks = m_terminalControllerCompPtr->GetOutput(
				sessionId, fromSequence, nextSequence, running, finished, exitCode);

	response.Version_1_0.emplace();
	response.Version_1_0->sessionId = sessionId;
	response.Version_1_0->nextSequence = nextSequence;
	response.Version_1_0->running = running;
	response.Version_1_0->finished = finished;
	response.Version_1_0->exitCode = exitCode;

	QList<sdl::agentino::Terminal::CTerminalOutputChunk::V1_0> chunkList;
	for (const agentinodata::ITerminalController::OutputChunk& chunk: chunks){
		sdl::agentino::Terminal::CTerminalOutputChunk::V1_0 chunkItem;
		chunkItem.sequence = chunk.sequence;
		chunkItem.stream = (sdl::agentino::Terminal::TerminalStreamType) chunk.stream;
		chunkItem.data = chunk.data;

		chunkList << chunkItem;
	}

	response.Version_1_0->chunks.Emplace();
	response.Version_1_0->chunks->FromList(chunkList);

	return response;
}


sdl::agentino::Terminal::COpenTerminalSessionResponse CTerminalControllerComp::OnOpenTerminalSession(
			const sdl::agentino::Terminal::COpenTerminalSessionGqlRequest& openTerminalSessionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::COpenTerminalSessionResponse response;

	if (!m_terminalControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TerminalController' was not set", "CTerminalControllerComp");

		return response;
	}

	sdl::agentino::Terminal::OpenTerminalSessionRequestArguments arguments = openTerminalSessionRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CTerminalControllerComp");

		return response;
	}

	if (!arguments.input.Version_1_0->shellType.has_value()){
		errorMessage = QString("Unable to open terminal session without a shell type");

		return response;
	}

	agentinodata::ITerminalController::ShellType shellType =
				(agentinodata::ITerminalController::ShellType) *arguments.input.Version_1_0->shellType;

	QByteArray sessionId = m_terminalControllerCompPtr->OpenSession(shellType, errorMessage);

	response.Version_1_0.emplace();
	response.Version_1_0->sessionId = sessionId;
	response.Version_1_0->shellType = *arguments.input.Version_1_0->shellType;
	response.Version_1_0->started = !sessionId.isEmpty();

	return response;
}


sdl::agentino::Terminal::CSendTerminalInputResponse CTerminalControllerComp::OnSendTerminalInput(
			const sdl::agentino::Terminal::CSendTerminalInputGqlRequest& sendTerminalInputRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CSendTerminalInputResponse response;

	if (!m_terminalControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TerminalController' was not set", "CTerminalControllerComp");

		return response;
	}

	sdl::agentino::Terminal::SendTerminalInputRequestArguments arguments = sendTerminalInputRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CTerminalControllerComp");

		return response;
	}

	response.Version_1_0.emplace();
	response.Version_1_0->accepted = false;

	if (!arguments.input.Version_1_0->sessionId.has_value()){
		errorMessage = QString("Unable to send terminal input with empty session ID");

		return response;
	}

	QByteArray sessionId = *arguments.input.Version_1_0->sessionId;
	QString data;
	if (arguments.input.Version_1_0->data.has_value()){
		data = *arguments.input.Version_1_0->data;
	}

	response.Version_1_0->accepted = m_terminalControllerCompPtr->SendInput(sessionId, data);

	return response;
}


sdl::agentino::Terminal::CCloseTerminalSessionResponse CTerminalControllerComp::OnCloseTerminalSession(
			const sdl::agentino::Terminal::CCloseTerminalSessionGqlRequest& closeTerminalSessionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Terminal::CCloseTerminalSessionResponse response;

	if (!m_terminalControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TerminalController' was not set", "CTerminalControllerComp");

		return response;
	}

	sdl::agentino::Terminal::CloseTerminalSessionRequestArguments arguments = closeTerminalSessionRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CTerminalControllerComp");

		return response;
	}

	response.Version_1_0.emplace();
	response.Version_1_0->closed = false;

	if (!arguments.input.Version_1_0->sessionId.has_value()){
		errorMessage = QString("Unable to close terminal session with empty session ID");

		return response;
	}

	QByteArray sessionId = *arguments.input.Version_1_0->sessionId;
	response.Version_1_0->closed = m_terminalControllerCompPtr->CloseSession(sessionId);

	return response;
}


} // namespace agentgql
