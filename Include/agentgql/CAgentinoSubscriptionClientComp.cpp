#include <agentgql/CAgentinoSubscriptionClientComp.h>


// Qt includes
#include <QtNetwork/QHostInfo>

// ImtCore includes
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlResponse.h>


namespace agentgql
{


// protected methods

void CAgentinoSubscriptionClientComp::OnComponentCreated()
{
	if (m_webLoginStatusModelCompPtr.IsValid()){
		m_webLoginStatusModelCompPtr->AttachObserver(this);
	}
}


// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CAgentinoSubscriptionClientComp::OnResponseReceived(const QByteArray& /*subscriptionId*/, const QByteArray& /*subscriptionData*/)
{
}


void CAgentinoSubscriptionClientComp::OnSubscriptionStatusChanged(const QByteArray& /*subscriptionId*/, const SubscriptionStatus& /*status*/, const QString& /*message*/)
{
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentinoSubscriptionClientComp::OnUpdate(const istd::IChangeable::ChangeSet& /*changeSet*/)
{
	if (!m_loginStatusCompPtr.IsValid() || !m_gqlClientCompPtr.IsValid() || !m_applicationInfoCompPtr.IsValid()){
		return;
	}

	imtcom::IConnectionStatusProvider::ConnectionStatus connectionStatus = m_loginStatusCompPtr->GetConnectionStatus();
	if (connectionStatus == imtcom::IConnectionStatusProvider::CS_CONNECTED){
		imtgql::CGqlRequest* gqlInitRequest = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_MUTATION, "AgentAdd");
		imtgql::CGqlParamObject inputDataParams;
		QString clientId;
		if (m_clientIdCompPtr.IsValid()){
			clientId = m_clientIdCompPtr->GetText();
		}
		inputDataParams.InsertParam("id", QVariant(clientId));

		QString localHostName = QHostInfo::localHostName();
		QString domainMain = QHostInfo::localDomainName();

		QString name = localHostName;
		if (!domainMain.isEmpty()){
			name += "@" + domainMain;
		}

		QString version = m_applicationInfoCompPtr->GetApplicationAttribute(ibase::IApplicationInfo::AA_MAIN_VERSION);

		QJsonObject item;

		item.insert("name", name);
		item.insert("computerName", name);
		item.insert("httpUrl", "http://localhost:7222");
		item.insert("webSocketUrl", "http://localhost:7223");
		item.insert("version", version);

		QJsonDocument itemDocument;
		itemDocument.setObject(item);

		inputDataParams.InsertParam("item", QVariant(itemDocument.toJson(QJsonDocument::Compact)));
		gqlInitRequest->AddParam("input", inputDataParams);

		imtgql::CGqlFieldObject returnNotify;
		returnNotify.InsertField("status");
		gqlInitRequest->AddField("addedNotification", returnNotify);

		imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlInitRequest);
		imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_gqlClientCompPtr->SendRequest(requestPtr);
	}
}


} // namespace agentgql


