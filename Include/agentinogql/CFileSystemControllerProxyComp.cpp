// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CFileSystemControllerProxyComp.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/FileSystem.h>

// ImtCore includes
#include <imtgql/CGqlRequest.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)

sdl::V1_0::imtbase::CGetFileSystemEntriesPayload CFileSystemControllerProxyComp::OnGetFileSystemEntries(
			const sdl::V1_0::imtbase::CGetFileSystemEntriesGqlRequest& /*getFileSystemEntriesRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	// Require clientid so the request is routed to a concrete agent.
	// HTTP layer lowercases header names; accept a few aliases defensively.
	const imtgql::IGqlContext* contextPtr = gqlRequest.GetRequestContext();
	QByteArray clientId;
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


} // namespace agentinogql
