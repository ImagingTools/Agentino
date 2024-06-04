#include <agentinogql/CServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>


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
		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");

		QByteArray itemData;
		QByteArray agentId;
		QByteArray objectId;
		if (inputParamPtr != nullptr){
			objectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();
			itemData = inputParamPtr->GetFieldArgumentValue("Item").toByteArray();

			const imtgql::CGqlObject* addition = inputParamPtr->GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
			}
		}

		bool ok = false;
		imtbase::CTreeItemModel itemModel;
		if (gqlRequest.GetCommandId() == "ServiceAdd"){
			if (resultModelPtr.IsValid() && resultModelPtr->ContainsKey("item")){
				imtbase::CTreeItemModel* dataModelPtr = resultModelPtr->GetTreeItemModel("item");
				if (dataModelPtr != nullptr){
					itemModel.Copy(dataModelPtr);

					ok = true;
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


