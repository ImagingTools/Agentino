#include <agentgql/CAgentinoSubscriptionClientComp.h>


// ImtCore includes
#include <imtbase/CTreeItemModel.h>
#include <imtbase/ICollectionInfo.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/IServiceController.h>


namespace agentgql
{


// protected methods

void CAgentinoSubscriptionClientComp::OnComponentCreated()
{

	if (m_webLoginStatusModelCompPtr.IsValid()) {
		m_webLoginStatusModelCompPtr->AttachObserver(this);
	}
}


void CAgentinoSubscriptionClientComp::OnResponseReceived(const QByteArray & subscriptionId, const QByteArray & subscriptionData)
{
//	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
//	agentinodata::IServiceController::NotifierStatusInfo notifierInfo;
//	QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
//	QJsonObject subscriptionObject = document.object();
//	if (subscriptionId == m_serviceStatusSubsriptionId){
//		notifierInfo.serviceId = subscriptionObject.value("id").toString().toUtf8();
//		QString status = subscriptionObject.value("status").toString();
//		changeSet.SetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierInfo));
//	}
//	istd::CChangeNotifier notifier(this, &changeSet);

//	if (m_gqlCollectionCompPtr.IsValid()){
//		istd::CChangeNotifier notifier(m_gqlCollectionCompPtr.GetPtr(), &changeSet);
//	}
}


void CAgentinoSubscriptionClientComp::OnSubscriptionStatusChanged(const QByteArray & subscriptionId, const SubscriptionStatus & status, const QString & message)
{
}


void CAgentinoSubscriptionClientComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_loginStatusCompPtr.IsValid() || !m_gqlClientCompPtr.IsValid()){
		return;
	}
	imtauth::ILoginStatusProvider::LoginStatusFlags loginStatus = (imtauth::ILoginStatusProvider::LoginStatusFlags)m_loginStatusCompPtr->GetLoginStatus();

	if (loginStatus == imtauth::ILoginStatusProvider::LSF_LOGGED_IN){
		imtgql::CGqlRequest gqlInitRequest(imtgql::IGqlRequest::RT_MUTATION, "AgentUpdate");
		imtgql::CGqlObject inputDataParams("input");
		inputDataParams.InsertField("Id", QVariant("1111"));

		QJsonObject item;
		item.insert("Name", "Test");
		item.insert("Description", "Test description");
		item.insert("HttpUrl", "http://localhost:7222");
		item.insert("WebSocketUrl", "http://localhost:7223");

		QJsonDocument itemDocument;
		itemDocument.setObject(item);

//		imtgql::CGqlObject itemData("Item");
//		itemData.InsertField("Name", QVariant("Test"));
//		itemData.InsertField("Description", QVariant("Test description"));
//		itemData.InsertField("HttpUrl", QVariant("htp://localhost:7222"));
//		itemData.InsertField("WebSocketUrl", QVariant("htp://localhost:7223"));

		inputDataParams.InsertField("Item", QVariant(itemDocument.toJson(QJsonDocument::Compact)));
		gqlInitRequest.AddParam(inputDataParams);

		imtgql::CGqlObject returnNotify("updatedNotification");
//		returnNotify.InsertField("status");
		gqlInitRequest.AddField(returnNotify);

		Response response;
		bool retVal = m_gqlClientCompPtr->SendRequest(gqlInitRequest, response);
	}

}


// reimplemented (imtgql::IGqlClient::ResponseHandler)

CAgentinoSubscriptionClientComp::Response::Response()
{

}


QVariant CAgentinoSubscriptionClientComp::Response::GetResult() const
{
	return m_replyResult;
}


void CAgentinoSubscriptionClientComp::Response::OnReply(const imtgql::IGqlRequest& request, const QByteArray& replyData)
{
	QJsonDocument document = QJsonDocument::fromJson(replyData);
	m_replyResult = replyData;
}



} // namespace agentgql


