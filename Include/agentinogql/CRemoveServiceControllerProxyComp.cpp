#include <agentinogql/CRemoveServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::CGqlRequestHandlerCompBase)

imtbase::CTreeItemModel* CRemoveServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (m_serviceManagerCompPtr.IsValid()){
		QByteArray agentId = gqlRequest.GetHeader("clientid");
		QByteArray serviceId;

		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
		if (inputParamPtr != nullptr){
			serviceId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();
		}

		if (m_serviceManagerCompPtr->ServiceExists(agentId, serviceId)){
			m_serviceManagerCompPtr->RemoveService(agentId, serviceId);
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
}


} // namespace agentinogql


