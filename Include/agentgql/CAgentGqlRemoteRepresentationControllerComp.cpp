#include <agentgql/CAgentGqlRemoteRepresentationControllerComp.h>


// ImtCore includes
#include <imtgql/CGqlResponse.h>
#include <imtgql/CGqlContext.h>
#include <imtbase/IObjectCollection.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtbase/CUrlParam.h>


namespace agentgql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)
bool CAgentGqlRemoteRepresentationControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool retVal = BaseClass::IsRequestSupported(gqlRequest);
	if (retVal){
		QByteArray agentId = gqlRequest.GetHeader("clientid");
		if (!agentId.isEmpty()){
			return true;
		}
	}

	return false;
}


// reimplemented (imtgql::CGqlRepresentationDataControllerComp)

imtbase::CTreeItemModel* CAgentGqlRemoteRepresentationControllerComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& /*errorMessage*/) const
{
	if (!IsRequestSupported(gqlRequest) || !m_connectionCollectionProviderCompPtr.IsValid()){
		return nullptr;
	}

	istd::TUniqueInterfacePtr<imtgql::CGqlRequest> gqlRequestPtr;
	gqlRequestPtr.MoveCastedPtr(gqlRequest.CloneMe());
	if (!gqlRequestPtr.IsValid()){
		return nullptr;
	}

	QByteArray serviceId = gqlRequestPtr->GetHeader("serviceid");
	QByteArray token = gqlRequestPtr->GetHeader("token");
	imtgql::IGqlContext* gqlContext = const_cast<imtgql::IGqlContext*>(gqlRequestPtr->GetRequestContext());
	if (gqlContext != nullptr){
		gqlContext->SetToken(token);
	}

	QUrl url;
	QByteArray serviceTypeName;
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollection = m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServiceId(serviceId);
	if (connectionCollection != nullptr) {
		const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollection->GetServerConnectionList());
		const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
		if (objectCollection != nullptr) {
			imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
			for (const QByteArray& id : ids) {
				const imtservice::IServiceConnectionInfo* connectionParamPtr = connectionCollection->GetConnectionMetaInfo(id);
				if (connectionParamPtr == nullptr) {
					continue;
				}

				if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT) {
					serviceTypeName = connectionCollection->GetServiceTypeId().toUtf8();
					const imtcom::IServerConnectionInterface& serverConnectionInterface = connectionParamPtr->GetDefaultInterface();
					if (serverConnectionInterface.GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, url)) {
						break;
					}
				}
			}
		}
	}

	url.setPath("/" + serviceTypeName + "/graphql");
	imtbase::CUrlParam urlParam;
	urlParam.SetUrl(url);

	imtclientgql::IGqlClient::GqlRequestPtr clientRequestPtr;
	clientRequestPtr.MoveCastedPtr(gqlRequestPtr->CloneMe());
	if (clientRequestPtr.IsValid()) {
		imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_apiClientCompPtr->SendRequest(clientRequestPtr, &urlParam);
		if (responsePtr.IsValid()) {
			return CreateTreeItemModelFromResponse(gqlRequest.GetCommandId(), *responsePtr);
		}
	}

	return nullptr;
}


} // namespace agentgql


