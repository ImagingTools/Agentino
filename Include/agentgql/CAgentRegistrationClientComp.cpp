// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentRegistrationClientComp.h>


// Qt includes
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSysInfo>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

// ImtCore includes
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlResponse.h>


namespace agentgql
{


// protected methods

void CAgentRegistrationClientComp::OnComponentCreated()
{
	if (m_webLoginStatusModelCompPtr.IsValid()){
		m_webLoginStatusModelCompPtr->AttachObserver(this);
	}

	const int intervalSec = m_reannounceIntervalSecAttrPtr.IsValid() ? *m_reannounceIntervalSecAttrPtr : 0;
	if (intervalSec > 0){
		connect(&m_reannounceTimer, &QTimer::timeout, this, &CAgentRegistrationClientComp::Announce);
		m_reannounceTimer.start(intervalSec * 1000);
	}
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentRegistrationClientComp::OnUpdate(const istd::IChangeable::ChangeSet& /*changeSet*/)
{
	Announce();
}


// private methods

void CAgentRegistrationClientComp::Announce()
{
	if (!m_loginStatusCompPtr.IsValid() || !m_gqlClientCompPtr.IsValid() || !m_applicationInfoCompPtr.IsValid()
				|| !m_serverConnectionInterfaceCompPtr.IsValid()){
		return;
	}

	// Only announce while the link to the server is actually up. Repeated on a timer (see
	// ReannounceIntervalSec) - not just on this transition - because a GQL-level Deny never
	// closes the socket, so this is also how a later server-side ResetRejectedAgent/Approve/
	// Suspend decision reaches the agent without an actual reconnect.
	if (m_loginStatusCompPtr->GetConnectionStatus() != imtcom::IConnectionStatusProvider::CS_CONNECTED){
		return;
	}

	const QString localHostName = QHostInfo::localHostName();
	const QString domainName = QHostInfo::localDomainName();
	QString name = localHostName;
	if (!domainName.isEmpty()){
		name += "@" + domainName;
	}

	const QByteArray agentId = ResolveAgentId(name);
	if (agentId.isEmpty()){
		SendErrorMessage(
					0,
					QStringLiteral("Unable to resolve a stable agent id; registration skipped"),
					"CAgentRegistrationClientComp");

		return;
	}

	const QString version = m_applicationInfoCompPtr->GetApplicationAttribute(ibase::IApplicationInfo::AA_MAIN_VERSION);

	// This agent's own advertised address, as configured (see ServerConnectionInterface in
	// ServerSettings.acc) - never hardcoded, since it differs per deployment and per agent.
	// GetUrl() picks http/ws vs. https/wss from the interface's CF_SECURE flag.
	QUrl httpUrl;
	m_serverConnectionInterfaceCompPtr->GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, httpUrl);
	QUrl webSocketUrl;
	m_serverConnectionInterfaceCompPtr->GetUrl(imtcom::IServerConnectionInterface::PT_WEBSOCKET, webSocketUrl);

	QJsonObject item;
	item.insert("name", name);
	item.insert("computerName", name);
	item.insert("httpUrl", httpUrl.toString());
	item.insert("webSocketUrl", webSocketUrl.toString());
	item.insert("version", version);
	item.insert("os", QSysInfo::prettyProductName());

	QJsonDocument itemDocument;
	itemDocument.setObject(item);

	imtgql::CGqlParamObject inputDataParams;
	inputDataParams.InsertParam("collectionId", QVariant("Agents"));
	inputDataParams.InsertParam("id", QVariant(QString::fromUtf8(agentId)));
	inputDataParams.InsertParam("item", QVariant(itemDocument.toJson(QJsonDocument::Compact)));

	imtgql::CGqlRequest* gqlInitRequest = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_MUTATION, "AgentAdd");
	gqlInitRequest->AddParam("input", inputDataParams);

	imtgql::CGqlFieldObject returnNotify;
	returnNotify.InsertField("status");
	gqlInitRequest->AddField("addedNotification", returnNotify);

	imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlInitRequest);
	imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_gqlClientCompPtr->SendRequest(requestPtr);
	if (!responsePtr.IsValid()){
		SendErrorMessage(
					0,
					QString("Agent '%1' could not register on the server (no response to 'AgentAdd'); "
								"will retry on the next re-announce.")
								.arg(QString::fromUtf8(agentId)),
					"CAgentRegistrationClientComp");
	}
}


QByteArray CAgentRegistrationClientComp::ResolveAgentId(const QString& hostName) const
{
	if (m_clientIdCompPtr.IsValid()){
		const QString configuredId = m_clientIdCompPtr->GetText();
		if (!configuredId.isEmpty()){
			return configuredId.toUtf8();
		}
	}

	// Fallback: the host name is stable across restarts, which is what enrollment needs.
	return hostName.toUtf8();
}


} // namespace agentgql
