// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CGqlRepresentationAgentDataComp.h>


// Qt includes
#include <QtCore/QJsonObject>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::CGqlRequestHandlerCompBase)

QJsonObject CGqlRepresentationAgentDataComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	imtgql::IGqlRequest::RequestType requestType = gqlRequest.GetRequestType();

	if (requestType == imtgql::IGqlRequest::RT_QUERY){

		QJsonObject rootObj;
		QJsonObject dataObj;

		QString clientId;
		if (m_clientIdCompPtr.IsValid()){
			clientId = m_clientIdCompPtr->GetText();
		}
		dataObj.insert(QStringLiteral("clientid"), clientId);
		dataObj.insert(QStringLiteral("computername"), QStringLiteral("COMPUTER-NAME"));
		rootObj.insert(QStringLiteral("data"), dataObj);

		return rootObj;
	}

	errorMessage = QString("Unable to create internal response with command %1").arg(qPrintable(commandId));

	SendErrorMessage(0, errorMessage);

	Q_ASSERT(false);

	return QJsonObject();
}



} // namespace agentinogql


