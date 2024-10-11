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
		QByteArray agentId = gqlRequest.GetHeader("clientId");

		if (!agentId.isEmpty()){
			return true;
		}
	}

	return false;
}


} // namespace agentinogql


