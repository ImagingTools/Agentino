#include <agentinogql/CServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	istd::TDelPtr<imtbase::CTreeItemModel> resultModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
	if (resultModelPtr.IsValid()){
		if (resultModelPtr->ContainsKey("errors")){
			imtbase::CTreeItemModel* errorsModelPtr = resultModelPtr->GetTreeItemModel("errors");
			if (errorsModelPtr != nullptr){
				if (errorsModelPtr->ContainsKey("message")){
					errorMessage = errorsModelPtr->GetData("message").toString();
				}
			}

			return nullptr;
		}
	}

	if (m_serviceManagerCompPtr.IsValid()){
		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");

		QByteArray agentId = gqlRequest.GetHeader("clientid");
		QByteArray itemData;
		QByteArray objectId;
		if (inputParamPtr != nullptr){
			objectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();
			itemData = inputParamPtr->GetFieldArgumentValue("Item").toByteArray();
		}

		bool ok = false;
		imtbase::CTreeItemModel itemModel;
		if (gqlRequest.GetCommandId() == "ServiceAdd"){
			if (resultModelPtr.IsValid() && resultModelPtr->ContainsKey("item")){
				imtbase::CTreeItemModel* dataModelPtr = resultModelPtr->GetTreeItemModel("item");
				if (dataModelPtr != nullptr){
					itemModel.Copy(dataModelPtr);

					ok = true;

					if (m_serviceStatusCollectionCompPtr.IsValid()){
						if (dataModelPtr->ContainsKey("Id")){
							QByteArray serviceId = dataModelPtr->GetData("Id").toByteArray();

							istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
							serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

							serviceStatusInfoPtr->SetServiceId(serviceId);
							serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

							QByteArray retVal = m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
							if (retVal.isEmpty()){
								SendErrorMessage(0, QString("Unable to insert new status for service '%1'").arg(qPrintable(serviceId)), "CServiceControllerProxyComp");
							}
						}
					}
				}
			}
		}

		if (!ok){
			if (!itemModel.CreateFromJson(itemData)){
				return nullptr;
			}
		}

		QString serviceName;
		if (itemModel.ContainsKey("Name")){
			serviceName = itemModel.GetData("Name").toString();
		}

		QString serviceDescription;
		if (itemModel.ContainsKey("Description")){
			serviceDescription = itemModel.GetData("Description").toString();
		}

		istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
		serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

		if (!m_serviceInfoRepresentationController.GetDataModelFromRepresentation(itemModel, *serviceInfoPtr)){
			errorMessage = QString("Unable to get service info from representation model");
			SendErrorMessage(0, errorMessage);

			return nullptr;
		}

		serviceInfoPtr->SetObjectUuid(objectId);

		istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr = new imtbase::CTreeItemModel;
		imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");
		imtbase::CTreeItemModel* notificationModelPtr = nullptr;

		if (m_serviceManagerCompPtr->ServiceExists(agentId, objectId)){
			m_serviceManagerCompPtr->SetService(agentId, objectId, *serviceInfoPtr.PopPtr(), serviceName, serviceDescription);

			notificationModelPtr = dataModelPtr->AddTreeModel("updatedNotification");
		}
		else{
			m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr.PopPtr(), objectId, serviceName, serviceDescription);

			notificationModelPtr = dataModelPtr->AddTreeModel("addedNotification");
		}

		if (notificationModelPtr != nullptr){
			notificationModelPtr->SetData("Id", objectId);
			notificationModelPtr->SetData("Name", serviceName);
		}

		return rootModelPtr.PopPtr();
	}

	return resultModelPtr.PopPtr();
}


} // namespace agentinogql


