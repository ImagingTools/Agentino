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
	if (m_webLoginStatusModelCompPtr.IsValid()) {
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

void CAgentinoSubscriptionClientComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_loginStatusCompPtr.IsValid() || !m_gqlClientCompPtr.IsValid() || !m_applicationInfoCompPtr.IsValid()){
		return;
	}

	imtcom::IConnectionStatusProvider::ConnectionStatus connectionStatus = m_loginStatusCompPtr->GetConnectionStatus();
	if (connectionStatus == imtcom::IConnectionStatusProvider::CS_CONNECTED){
		imtgql::CGqlRequest* gqlInitRequest = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_MUTATION, "AgentAdd");
		imtgql::CGqlObject inputDataParams;
		QString clientId;
		if (m_clientIdCompPtr.IsValid()){
			clientId = m_clientIdCompPtr->GetText();
		}
		inputDataParams.InsertField("Id", QVariant(clientId));

		QString localHostName = QHostInfo::localHostName();
		QString domainMain = QHostInfo::localDomainName();

		QString name = localHostName;
		if (!domainMain.isEmpty()){
			name += "@" + domainMain;
		}

		QString version = m_applicationInfoCompPtr->GetApplicationAttribute(ibase::IApplicationInfo::AA_MAIN_VERSION);

		QJsonObject item;

		item.insert("Name", name);
		item.insert("ComputerName", name);
		item.insert("HttpUrl", "http://localhost:7222");
		item.insert("WebSocketUrl", "http://localhost:7223");
		item.insert("Version", version);

		QJsonDocument itemDocument;
		itemDocument.setObject(item);

		inputDataParams.InsertField("Item", QVariant(itemDocument.toJson(QJsonDocument::Compact)));
		gqlInitRequest->AddParam("input", inputDataParams);

		imtgql::CGqlObject returnNotify;
		returnNotify.InsertField("status");
		gqlInitRequest->AddField("addedNotification", returnNotify);

		imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlInitRequest);
		imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_gqlClientCompPtr->SendRequest(requestPtr);
	}
}


} // namespace agentgql


