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
		QByteArray agentId;
		const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParamObject("input");
		if (gqlInputParamPtr != nullptr){
			const imtgql::CGqlObject* addition = gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
			}
		}
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

	imtgql::CGqlRequest* gqlRequestPtr = dynamic_cast<imtgql::CGqlRequest*>(gqlRequest.CloneMe());
	if (gqlRequestPtr == nullptr){
		return nullptr;
	}

	imtclientgql::IGqlClient::GqlRequestPtr clientRequestPtr(dynamic_cast<imtgql::IGqlRequest*>(gqlRequestPtr));
	if (!clientRequestPtr.isNull()){
		QByteArray serviceId;
		QByteArray token;

		const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParamObject("input");
		if (gqlInputParamPtr != nullptr){
			const imtgql::CGqlObject* addition = gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				serviceId = addition->GetFieldArgumentValue("serviceId").toByteArray();
				token = addition->GetFieldArgumentValue("token").toByteArray();
				imtgql::CGqlContext* sessionGqlContextPtr = dynamic_cast<imtgql::CGqlContext*>(gqlRequestPtr->GetRequestContext());
				if (sessionGqlContextPtr == nullptr){
					sessionGqlContextPtr = new imtgql::CGqlContext();
				}
				sessionGqlContextPtr->SetToken(token);
				gqlRequestPtr->SetGqlContext(sessionGqlContextPtr);
			}
		}
		QUrl url;
		QByteArray serviceTypeName;
		std::shared_ptr<imtservice::IConnectionCollection> connectionCollection = m_connectionCollectionProviderCompPtr->GetConnectionCollection(serviceId);
		if (connectionCollection != nullptr){
			const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollection->GetUrlList());
			const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
			if (objectCollection != nullptr){
				imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
				for (const QByteArray& id: ids){
					const imtservice::IServiceConnectionParam* connectionParamPtr = connectionCollection->GetConnectionMetaInfo(id);
					if (connectionParamPtr == nullptr){
						continue;
					}

					if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
						QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
						QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
						serviceTypeName = connectionCollection->GetServiceTypeName().toUtf8();
						url = connectionParamPtr->GetDefaultUrl();

						imtbase::IObjectCollection::DataPtr dataPtr;
						objectCollection->GetObjectData(id, dataPtr);
						imtservice::CUrlConnectionParam* connectionParam = dynamic_cast<imtservice::CUrlConnectionParam*>(dataPtr.GetPtr());
						if (connectionParam != nullptr){
							url = connectionParam->GetUrl();
						}

						if (url.scheme() == "http"){
							break;
						}
					}

				}
			}
		}
		url.setPath("/" + serviceTypeName + "/graphql");
		imtbase::CUrlParam urlParam;
		urlParam.SetUrl(url);
		imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_apiClientCompPtr->SendRequest(clientRequestPtr, &urlParam);
		if (!responsePtr.isNull()){
			return CreateTreeItemModelFromResponse(*responsePtr);
		}
	}

	return nullptr;
}


} // namespace agentgql


