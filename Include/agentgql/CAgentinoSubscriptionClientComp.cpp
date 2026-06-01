// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentinoSubscriptionClientComp.h>


// Qt includes
#include <QtNetwork/QHostInfo>

// ImtCore includes
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlResponse.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>


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
		inputDataParams.InsertParam("collectionId", QVariant("Agents"));

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

		// Send local topology to the server on (re)connect
		SendTopologySync();
	}
}


// private methods

void CAgentinoSubscriptionClientComp::SendTopologySync() const
{
	if (!m_serviceCollectionCompPtr.IsValid() || !m_gqlClientCompPtr.IsValid()){
		return;
	}

	QString clientId;
	if (m_clientIdCompPtr.IsValid()){
		clientId = m_clientIdCompPtr->GetText();
	}

	// Build topology sync data as a JSON array of service information
	QJsonArray servicesArray;

	imtbase::ICollectionInfo::Ids serviceIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			continue;
		}

		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		QJsonObject serviceObj;
		serviceObj.insert("id", QString::fromUtf8(serviceId));
		serviceObj.insert("name", serviceInfoPtr->GetServiceName());
		serviceObj.insert("description", serviceInfoPtr->GetServiceDescription());
		serviceObj.insert("typeId", serviceInfoPtr->GetServiceTypeId());
		serviceObj.insert("version", serviceInfoPtr->GetServiceVersion());
		serviceObj.insert("isAutoStart", serviceInfoPtr->IsAutoStart());

		// Add service status if controller is available
		if (m_serviceControllerCompPtr.IsValid()){
			agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceControllerCompPtr->GetServiceStatus(serviceId);
			QString statusStr;
			switch (status){
				case agentinodata::IServiceStatusInfo::SS_RUNNING:
					statusStr = "RUNNING";
					break;
				case agentinodata::IServiceStatusInfo::SS_NOT_RUNNING:
					statusStr = "NOT_RUNNING";
					break;
				case agentinodata::IServiceStatusInfo::SS_STARTING:
					statusStr = "STARTING";
					break;
				case agentinodata::IServiceStatusInfo::SS_STOPPING:
					statusStr = "STOPPING";
					break;
				case agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
					statusStr = "RUNNING_IMPOSSIBLE";
					break;
				default:
					statusStr = "UNDEFINED";
					break;
			}
			serviceObj.insert("status", statusStr);
		}

		servicesArray.append(serviceObj);
	}

	// Send topology sync mutation
	imtgql::CGqlRequest* gqlSyncRequest = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_MUTATION, "SyncAgentTopology");
	imtgql::CGqlParamObject syncInputParams;
	syncInputParams.InsertParam("agentId", QVariant(clientId));

	QJsonDocument servicesDocument;
	servicesDocument.setArray(servicesArray);
	syncInputParams.InsertParam("services", QVariant(servicesDocument.toJson(QJsonDocument::Compact)));

	gqlSyncRequest->AddParam("input", syncInputParams);

	imtgql::CGqlFieldObject returnField;
	returnField.InsertField("successful");
	gqlSyncRequest->AddField("syncResult", returnField);

	imtclientgql::IGqlClient::GqlRequestPtr syncRequestPtr(gqlSyncRequest);
	imtclientgql::IGqlClient::GqlResponsePtr syncResponsePtr = m_gqlClientCompPtr->SendRequest(syncRequestPtr);
}


} // namespace agentgql


