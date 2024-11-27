#include <agentinogql/CServiceStatusControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


imtbase::CTreeItemModel* CServiceStatusControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray serviceId;
	QByteArray commandId = gqlRequest.GetCommandId();

	if (m_serviceStatusCollectionCompPtr.IsValid()){
		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
		if (inputParamPtr != nullptr){
			serviceId = inputParamPtr->GetFieldArgumentValue("serviceid").toByteArray();
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
			istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
			serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

			serviceStatusInfoPtr->SetServiceStatus(status);
			serviceStatusInfoPtr->SetServiceId(serviceId);

			m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
		}
	}

	istd::TDelPtr<imtbase::CTreeItemModel> resultModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
	if (!resultModelPtr.IsValid() || (resultModelPtr.IsValid() && resultModelPtr->ContainsKey("errors"))){
		if (commandId == "ServiceStart"){
			errorMessage = QString("The service cannot be started check the agent connection");
		}
		else if (commandId == "ServiceStop"){
			errorMessage = QString("The service cannot be stopped check the agent connection");
		}

		return nullptr;
	}

	return resultModelPtr.PopPtr();
}


} // namespace agentinogql


