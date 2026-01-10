#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)
bool CAgentinoGqlRemoteRepresentationControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
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


} // namespace agentinogql


