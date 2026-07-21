// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QTimer>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <imod/TSingleModelObserverBase.h>
#include <iprm/ITextParam.h>
#include <ibase/IApplicationInfo.h>

// ImtCore includes
#include <imtclientgql/IGqlClient.h>
#include <imtcom/IConnectionStatusProvider.h>
#include <imtcom/IServerConnectionInterface.h>


namespace agentgql
{


/**
	Announces this agent to the central server.

	Watches the agent's own connection status and, every time the WebSocket link to
	the server comes up, sends one 'AgentAdd' mutation carrying the agent id and its
	descriptive attributes (host name, OS, version, advertised endpoints). It also
	repeats this on a timer while connected: a GQL-level Deny (Rejected/Revoked) never
	closes the socket, and nothing else re-triggers registration, so without the timer
	an operator's ResetRejectedAgent/Approve/Suspend would only reach the agent on its
	next actual reconnect (network blip, server or agent restart) - which may be never.
	Otherwise this component owns no collection and subscribes to nothing.

	The server turns this into an enrollment record — a first-seen agent lands in
	Pending and stays inert until an operator approves it (see IEnrollmentGate). The
	reported agent id therefore has to be **stable across restarts**, otherwise every
	restart would create a fresh Pending record. It comes from ClientIdParam, with the
	host name as a last-resort fallback when that is not configured.
*/
class CAgentRegistrationClientComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			public imod::CSingleModelObserverBase
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CAgentRegistrationClientComp);
		I_ASSIGN(m_gqlClientCompPtr, "GqlClient", "GraphQl client used to send the registration", true, "GqlClient");
		I_ASSIGN(m_loginStatusCompPtr, "WebLoginStatus", "Connection status to the central server", true, "WebLoginStatus");
		I_ASSIGN_TO(m_webLoginStatusModelCompPtr, m_loginStatusCompPtr, true);
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Stable id this agent reports to the server", false, "ClientIdParam");
		I_ASSIGN(m_applicationInfoCompPtr, "ApplicationInfo", "Application info", true, "ApplicationInfo");
		I_ASSIGN(m_serverConnectionInterfaceCompPtr, "ServerConnectionInterface", "This agent's own advertised host/HTTP/WS interface, reported to the server as-is", true, "ServerConnectionInterface");
		I_ASSIGN(m_reannounceIntervalSecAttrPtr, "ReannounceIntervalSec", "How often to resend AgentAdd while connected, so a server-side enrollment decision (approve/reject/reset) is picked up without an actual reconnect. 0 disables the timer (announce only on connect).", false, 60);
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

private:
	/**
		Stable identity this agent reports. Prefers ClientIdParam and falls back to
		\a hostName, so a misconfigured agent still keeps a single enrollment record
		instead of creating a new Pending entry on every restart.
	*/
	QByteArray ResolveAgentId(const QString& hostName) const;

	/** Send 'AgentAdd' if currently connected; no-op otherwise. Called on connect and on timer. */
	void Announce();

	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusCompPtr);
	I_REF(imod::IModel, m_webLoginStatusModelCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
	I_REF(ibase::IApplicationInfo, m_applicationInfoCompPtr);
	I_REF(imtcom::IServerConnectionInterface, m_serverConnectionInterfaceCompPtr);
	I_ATTR(int, m_reannounceIntervalSecAttrPtr);

	QTimer m_reannounceTimer;
};


} // namespace agentgql
