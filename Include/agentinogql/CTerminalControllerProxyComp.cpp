// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTerminalControllerProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal.h>


// Qt includes
#include <QtCore/QMutexLocker>

// ImtCore includes
#include <imtgql/CGqlRequest.h>
#include <imtgql/IGqlContext.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CTerminalGqlHandlerCompBase)

sdl::V1_0::agentino::CShellTypeListPayload CTerminalControllerProxyComp::OnListShellTypes(
			const sdl::V1_0::agentino::CListShellTypesGqlRequest& /*listShellTypesRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	// No sessionId involved - nothing to own-check.
	return ForwardToAgent<sdl::V1_0::agentino::CShellTypeListPayload>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CTerminalOutputResponse CTerminalControllerProxyComp::OnGetTerminalOutput(
			const sdl::V1_0::agentino::CGetTerminalOutputGqlRequest& /*getTerminalOutputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CTerminalOutputResponse();
	}

	sdl::V1_0::agentino::CTerminalOutputResponse retVal =
				ForwardToAgent<sdl::V1_0::agentino::CTerminalOutputResponse>(gqlRequest, errorMessage);

	// Opportunistic cleanup: the agent reports the session as gone (exited or evicted by
	// its own idle timeout), so this ownership entry can be forgotten instead of sitting
	// around forever - the same sessionId will never legitimately reappear.
	if (retVal.running.HasValue() && !retVal.running.GetValue()){
		const QByteArray sessionId = ExtractSessionId(gqlRequest);
		if (!sessionId.isEmpty()){
			QMutexLocker locker(&m_sessionOwnersMutex);
			m_sessionOwners.remove(sessionId);
		}
	}

	return retVal;
}


sdl::V1_0::agentino::COpenTerminalSessionResponse CTerminalControllerProxyComp::OnOpenTerminalSession(
			const sdl::V1_0::agentino::COpenTerminalSessionGqlRequest& /*openTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::COpenTerminalSessionResponse retVal =
				ForwardToAgent<sdl::V1_0::agentino::COpenTerminalSessionResponse>(gqlRequest, errorMessage);

	// The session only exists from this point on, and only this caller (the one who just
	// opened it) may be its owner - every later command against this sessionId is checked
	// against this entry (CheckSessionOwnership).
	if (errorMessage.isEmpty() && retVal.sessionId.HasValue() && !retVal.sessionId.GetValue().isEmpty()){
		const QByteArray userId = ExtractUserId(gqlRequest);
		QMutexLocker locker(&m_sessionOwnersMutex);
		m_sessionOwners.insert(retVal.sessionId.GetValue(), userId);
	}

	return retVal;
}


sdl::V1_0::agentino::CSendTerminalInputResponse CTerminalControllerProxyComp::OnSendTerminalInput(
			const sdl::V1_0::agentino::CSendTerminalInputGqlRequest& /*sendTerminalInputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CSendTerminalInputResponse();
	}

	return ForwardToAgent<sdl::V1_0::agentino::CSendTerminalInputResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CSendTerminalRawInputResponse CTerminalControllerProxyComp::OnSendTerminalRawInput(
			const sdl::V1_0::agentino::CSendTerminalRawInputGqlRequest& /*sendTerminalRawInputRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CSendTerminalRawInputResponse();
	}

	return ForwardToAgent<sdl::V1_0::agentino::CSendTerminalRawInputResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CInterruptTerminalSessionResponse CTerminalControllerProxyComp::OnInterruptTerminalSession(
			const sdl::V1_0::agentino::CInterruptTerminalSessionGqlRequest& /*interruptTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CInterruptTerminalSessionResponse();
	}

	return ForwardToAgent<sdl::V1_0::agentino::CInterruptTerminalSessionResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CResizeTerminalSessionResponse CTerminalControllerProxyComp::OnResizeTerminalSession(
			const sdl::V1_0::agentino::CResizeTerminalSessionGqlRequest& /*resizeTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CResizeTerminalSessionResponse();
	}

	return ForwardToAgent<sdl::V1_0::agentino::CResizeTerminalSessionResponse>(gqlRequest, errorMessage);
}


sdl::V1_0::agentino::CCloseTerminalSessionResponse CTerminalControllerProxyComp::OnCloseTerminalSession(
			const sdl::V1_0::agentino::CCloseTerminalSessionGqlRequest& /*closeTerminalSessionRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!CheckSessionOwnership(gqlRequest, errorMessage)){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return sdl::V1_0::agentino::CCloseTerminalSessionResponse();
	}

	sdl::V1_0::agentino::CCloseTerminalSessionResponse retVal =
				ForwardToAgent<sdl::V1_0::agentino::CCloseTerminalSessionResponse>(gqlRequest, errorMessage);

	// The caller is explicitly done with this sessionId either way (even an agent-side
	// close failure is treated as closed client-side, same as TerminalController.qml's own
	// forgetSession on a CloseTerminalSession error) - forget the owner now, not later.
	const QByteArray sessionId = ExtractSessionId(gqlRequest);
	if (!sessionId.isEmpty()){
		QMutexLocker locker(&m_sessionOwnersMutex);
		m_sessionOwners.remove(sessionId);
	}

	return retVal;
}


// private methods

template <class SdlResponse>
SdlResponse CTerminalControllerProxyComp::ForwardToAgent(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	// Require clientid so the shell is opened on a concrete agent instead of silently
	// hitting whatever the API client happens to be connected to.
	if (gqlRequest.GetHeader(QByteArrayLiteral("clientid")).isEmpty()){
		errorMessage = QStringLiteral(
					"Unable to serve terminal request. Error: request header 'clientid' (agent id) is missing");
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return SdlResponse();
	}

	// Permission check is performed by CPermissibleGqlRequestHandlerComp base
	// (CreateResponse -> CheckPermissions) before this method is invoked.
	SdlResponse retVal = SendModelRequest<SdlResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CTerminalControllerProxyComp");

		return SdlResponse();
	}

	return retVal;
}


QByteArray CTerminalControllerProxyComp::ExtractSessionId(const imtgql::CGqlRequest& gqlRequest)
{
	const imtgql::CGqlParamObject* inputPtr = gqlRequest.GetParamObject("input");
	if (inputPtr == nullptr){
		return QByteArray();
	}

	return inputPtr->GetParamArgumentValue("sessionId").toByteArray();
}


QByteArray CTerminalControllerProxyComp::ExtractUserId(const imtgql::CGqlRequest& gqlRequest)
{
	const imtgql::IGqlContext* gqlContextPtr = gqlRequest.GetRequestContext();

	return gqlContextPtr != nullptr ? gqlContextPtr->GetUserId() : QByteArray();
}


bool CTerminalControllerProxyComp::CheckSessionOwnership(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	const QByteArray sessionId = ExtractSessionId(gqlRequest);
	if (sessionId.isEmpty()){
		return true;
	}

	const QByteArray callerUserId = ExtractUserId(gqlRequest);

	QMutexLocker locker(&m_sessionOwnersMutex);

	const auto it = m_sessionOwners.constFind(sessionId);
	if (it == m_sessionOwners.constEnd()){
		// Fail-closed: a sessionId this instance never recorded (already closed, evicted,
		// or from before a server restart) must be refused, not merely warned about - the
		// alternative is indistinguishable from "anyone who guesses/logs a sessionId can
		// use it".
		errorMessage = QStringLiteral("Unknown or expired terminal session");

		return false;
	}

	if (it.value() != callerUserId){
		errorMessage = QStringLiteral("This terminal session belongs to another user");

		return false;
	}

	return true;
}


} // namespace agentinogql
