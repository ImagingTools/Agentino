#include <agentinogql/CServiceStatusControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


imtbase::CTreeItemModel* CServiceStatusControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (m_serviceStatusCollectionCompPtr.IsValid()){
		QByteArray commandId = gqlRequest.GetCommandId();
		QByteArray serviceId;

		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");
		if (inputParamPtr != nullptr){
			serviceId = inputParamPtr->GetFieldArgumentValue("serviceId").toByteArray();
		}

		agentinodata::IServiceStatusInfo::ServiceStatus status = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
		if (commandId == "ServiceStart"){
			status = agentinodata::IServiceStatusInfo::SS_STARTING;
		}
		else if (commandId == "ServiceStop"){
			status = agentinodata::IServiceStatusInfo::SS_STOPPING;
		}

		if (m_serviceStatusCollectionCompPtr->GetElementIds().contains(serviceId)){
			imtbase::IObjectCollection::DataPtr dataPtr;
			if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
				agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
				serviceStatusInfoPtr->SetServiceStatus(status);

				m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr);
			}
		}
		else{
			istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr = new agentinodata::CServiceStatusInfo;
			serviceStatusInfoPtr->SetServiceStatus(status);
			serviceStatusInfoPtr->SetServiceId(serviceId);

			m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
}


} // namespace agentinogql


