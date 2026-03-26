// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentSettingsControllerComp.h>


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>


namespace agentgql
{


QJsonObject CAgentSettingsControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_agentinoConnectionInterfaceCompPtr.IsValid()){
		Q_ASSERT(0);

		return QJsonObject();
	}

	if (!m_loginCompPtr.IsValid()){
		Q_ASSERT(0);

		return QJsonObject();
	}

	const imtgql::CGqlParamObject* gqlInputParamPtr = gqlRequest.GetParamObject("input");
	if (gqlInputParamPtr == nullptr){
		SendErrorMessage(0, QString("GraphQL input parameters is invalid"));

		return QJsonObject();
	}

	if (gqlRequest.GetRequestType() == imtgql::IGqlRequest::RT_QUERY){
		QJsonObject rootObj;
		QJsonObject dataObj;
		QUrl url;
		if (!m_agentinoConnectionInterfaceCompPtr->GetUrl(imtcom::IServerConnectionInterface::PT_WEBSOCKET, url)){
			SendErrorMessage(0, QString("ServerConnectionInterface is invalid"));

			return QJsonObject();
		}
		QString agentinoUrl = url.toString();
		dataObj.insert(QStringLiteral("Url"), agentinoUrl);
		rootObj.insert(QStringLiteral("data"), dataObj);

		return rootObj;
	}
	else if (gqlRequest.GetRequestType() == imtgql::IGqlRequest::RT_MUTATION){
		QByteArray agentinoUrl;

		QByteArray itemData = gqlInputParamPtr->GetParamArgumentValue("Item").toByteArray();

		QJsonDocument itemDoc = QJsonDocument::fromJson(itemData);
		if (!itemDoc.isObject()){
			SendErrorMessage(0, QString("Unable to create model from json: '%1'").arg(qPrintable(itemData)));

			return QJsonObject();
		}

		QJsonObject itemObj = itemDoc.object();

		if (itemObj.contains(QStringLiteral("Url"))){
			agentinoUrl = itemObj.value(QStringLiteral("Url")).toString().toUtf8();
		}

		if (agentinoUrl.isEmpty()){
			errorMessage = QString("URL cannot be empty");

			return QJsonObject();
		}

		QUrl url(agentinoUrl);

		if (!url.isValid()){
			errorMessage = QString("URL is invalid");

			return QJsonObject();
		}

		m_loginCompPtr->Disconnect();

		m_agentinoConnectionInterfaceCompPtr->SetHost(url.host());
		m_agentinoConnectionInterfaceCompPtr->SetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET,url.port());

		QJsonObject rootObj;
		QJsonObject dataObj;
		QJsonObject notificationObj;
		notificationObj.insert(QStringLiteral("Id"), QStringLiteral(""));
		notificationObj.insert(QStringLiteral("Name"), QStringLiteral("Agent Settings"));
		dataObj.insert(QStringLiteral("updatedNotification"), notificationObj);
		rootObj.insert(QStringLiteral("data"), dataObj);

		m_loginCompPtr->Connect();

		return rootObj;
	}

	return QJsonObject();
}


} // namespace agentgql


