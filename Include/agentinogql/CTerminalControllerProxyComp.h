// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QMutex>

// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Terminal_fwd.h>


namespace agentinogql
{


/**
	Server side proxy for the remote terminal feature.

	Runs inside the AgentinoServer process and forwards every terminal request to the
	agent identified by the request header \c clientid (same routing channel as
	\ref CServiceControllerProxyComp and \ref CFileSystemControllerProxyComp), returning
	the agent response unchanged. The actual shell process always runs on the agent
	machine.

	\note Session ownership: the RemoteTerminal permission alone only proves the caller
	may use *some* terminal, not that they may touch *this* sessionId - a session is only
	known to the agent by sessionId, with no per-caller identity at all (the server->agent
	path strips x-authentication-token), so ownership must be enforced here, the one place
	that sees both the authenticated caller (imtgql::IGqlContext::GetUserId(), via
	gqlRequest.GetRequestContext()) and every per-session command. OnOpenTerminalSession
	records the caller as the session's owner; every other per-session command checks it
	before forwarding, fail-closed (a sessionId this instance never recorded ownership for -
	e.g. after a server restart - is refused, not merely warned about).

	Ownership entries are released on CloseTerminalSession, on a GetTerminalOutput or a
	relayed OnTerminalOutputChanged push reporting the session as no longer running, and -
	as a backstop for sessions whose end is never observed - after
	\ref SessionOwnerTtlSeconds without any owned request or relayed output.

	\ingroup Terminal
*/
class CTerminalControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
										sdl::V1_0::agentino::CTerminalGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::V1_0::agentino::CTerminalGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CTerminalControllerProxyComp);
	I_END_COMPONENT;

	/**
		Forget the owner of \p sessionId. Called by \ref CTerminalOutputSubscriberProxyComp
		when a relayed output push reports the session as no longer running (natural shell
		exit or agent idle close) - the GUI then only forgets the session locally and never
		sends CloseTerminalSession, so without this the entry would never be released.
	*/
	static void ReleaseSessionOwnership(const QByteArray& sessionId);

	/**
		Mark \p sessionId as still in use (e.g. its output was just relayed), postponing
		expiry of its ownership entry. No-op for unknown sessions.
	*/
	static void TouchSessionOwnership(const QByteArray& sessionId);

protected:
	// reimplemented (sdl::V1_0::agentino::CTerminalGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CShellTypeListPayload OnListShellTypes(
				const sdl::V1_0::agentino::CListShellTypesGqlRequest& listShellTypesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTerminalOutputResponse OnGetTerminalOutput(
				const sdl::V1_0::agentino::CGetTerminalOutputGqlRequest& getTerminalOutputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::COpenTerminalSessionResponse OnOpenTerminalSession(
				const sdl::V1_0::agentino::COpenTerminalSessionGqlRequest& openTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSendTerminalInputResponse OnSendTerminalInput(
				const sdl::V1_0::agentino::CSendTerminalInputGqlRequest& sendTerminalInputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSendTerminalRawInputResponse OnSendTerminalRawInput(
				const sdl::V1_0::agentino::CSendTerminalRawInputGqlRequest& sendTerminalRawInputRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CInterruptTerminalSessionResponse OnInterruptTerminalSession(
				const sdl::V1_0::agentino::CInterruptTerminalSessionGqlRequest& interruptTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CResizeTerminalSessionResponse OnResizeTerminalSession(
				const sdl::V1_0::agentino::CResizeTerminalSessionGqlRequest& resizeTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CCloseTerminalSessionResponse OnCloseTerminalSession(
				const sdl::V1_0::agentino::CCloseTerminalSessionGqlRequest& closeTerminalSessionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		Forward the request to the agent addressed by the \c clientid header.
		\return The agent response, or a default constructed one when the header is
		missing or the agent answered with an error.
	*/
	template <class SdlResponse>
	SdlResponse ForwardToAgent(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;

	static QByteArray ExtractSessionId(const imtgql::CGqlRequest& gqlRequest);
	static QByteArray ExtractUserId(const imtgql::CGqlRequest& gqlRequest);

	/**
		\return true if \p gqlRequest's sessionId (if it has one) is owned by its caller;
		on false, \p errorMessage is set and the caller must not forward the request.
		A request with no sessionId param (e.g. ListShellTypes) always passes.
	*/
	bool CheckSessionOwnership(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;

	/**
		Drop every ownership entry not used for longer than \ref SessionOwnerTtlSeconds.
		Backstop for sessions whose end is never observed (no CloseTerminalSession, no
		subscriber listening when the shell exited). Caller must hold s_sessionOwnersMutex.
	*/
	static void PruneExpiredSessionOwnersLocked(const QDateTime& now);

private:
	struct SessionOwner
	{
		QByteArray userId;
		QDateTime lastActivity;
	};

	// Well above the agent's own idle timeout (15 min, CTerminalSessionManagerComp), so an
	// entry only expires once the agent has certainly dropped the session as well.
	static const int SessionOwnerTtlSeconds = 60 * 60;

	// Shared by all instances: each request-handler worker owns an instance of this
	// component, and a session opened through one is used through the others.
	static QMutex s_sessionOwnersMutex;
	static QMap<QByteArray, SessionOwner> s_sessionOwners; // sessionId -> owner
};


} // namespace agentinogql
