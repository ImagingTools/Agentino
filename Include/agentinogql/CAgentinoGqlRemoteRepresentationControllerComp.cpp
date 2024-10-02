#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>


// ImtCore includes
#include <imtgql/CGqlResponse.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)
bool CAgentinoGqlRemoteRepresentationControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
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


} // namespace agentinogql


