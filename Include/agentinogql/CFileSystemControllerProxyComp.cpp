// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CFileSystemControllerProxyComp.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/FileSystem.h>

// ImtCore includes
#include <imtgql/CGqlRequest.h>


namespace agentinogql
{


// public methods

// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)

QJsonObject CFileSystemControllerProxyComp::CreateInternalResponse(const ::imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	// No agent to route to: the file system of the server itself, if it may be browsed.
	if (GetClientId(gqlRequest).isEmpty() && m_localFileSystemControllerCompPtr.IsValid()){
		if (m_localFileSystemControllerCompPtr->IsRequestSupported(gqlRequest)){
			return m_localFileSystemControllerCompPtr->CreateResponse(gqlRequest, errorMessage);
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
}


// protected methods

// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)

sdl::V1_0::imtbase::CGetFileSystemEntriesPayload CFileSystemControllerProxyComp::OnGetFileSystemEntries(
			const sdl::V1_0::imtbase::CGetFileSystemEntriesGqlRequest& /*getFileSystemEntriesRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	// Require clientid so the request is routed to a concrete agent.
	const QByteArray clientId = GetClientId(gqlRequest);
	if (clientId.isEmpty()){
		errorMessage = QStringLiteral(
					"Unable to browse file system. Error: request header 'clientid' (agent id) is missing");
		SendErrorMessage(0, errorMessage, "CFileSystemControllerProxyComp");

		return sdl::V1_0::imtbase::CGetFileSystemEntriesPayload();
	}

	// Permission check is performed by CPermissibleGqlRequestHandlerComp base
	// (CreateResponse → CheckPermissions) before this method is invoked.
	return SendModelRequest<sdl::V1_0::imtbase::CGetFileSystemEntriesPayload>(gqlRequest, errorMessage);
}


// private methods

QByteArray CFileSystemControllerProxyComp::GetClientId(const ::imtgql::CGqlRequest& gqlRequest)
{
	// HTTP layer lowercases header names; accept a few aliases defensively.
	QByteArray clientId;

	const imtgql::IGqlContext* contextPtr = gqlRequest.GetRequestContext();
	if (contextPtr != nullptr){
		const imtgql::IGqlContext::Headers headers = contextPtr->GetHeaders();
		clientId = headers.value(QByteArrayLiteral("clientid"));
		if (clientId.isEmpty()){
			clientId = headers.value(QByteArrayLiteral("clientId"));
		}
		if (clientId.isEmpty()){
			clientId = headers.value(QByteArrayLiteral("ClientId"));
		}
		if (clientId.isEmpty()){
			for (auto it = headers.constBegin(); it != headers.constEnd(); ++it){
				if (QString::fromUtf8(it.key()).compare(QStringLiteral("clientid"), Qt::CaseInsensitive) == 0){
					clientId = it.value();
					break;
				}
			}
		}
	}

	return clientId;
}


} // namespace agentinogql


