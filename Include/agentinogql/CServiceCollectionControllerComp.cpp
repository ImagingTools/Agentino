#include <agentinogql/CServiceCollectionControllerComp.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/agentinodata.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::CObjectCollectionControllerCompBase)

bool CServiceCollectionControllerComp::SetupGqlItem(
			const imtgql::CGqlRequest& gqlRequest,
			imtbase::CTreeItemModel& model,
			int itemIndex,
			const QByteArray& collectionId,
			QString& errorMessage) const
{
	QByteArray agentId;
	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		const imtgql::CGqlObject* addition =gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
		}
	}

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty()){
		agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (serviceCollectionPtr->GetObjectData(collectionId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr){
			QByteArray serviceId;
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = serviceCollectionPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "Id"){
					serviceId = collectionId;
					elementInformation = serviceId;
				}
				else if(informationId == "Name"){
					elementInformation = serviceCollectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_NAME).toString();
				}
				else if(informationId == "Description"){
					elementInformation = serviceCollectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
				}
				else if(informationId == "Path"){
					elementInformation = serviceInfoPtr->GetServicePath();
				}
				else if(informationId == "SettingsPath"){
					elementInformation = serviceInfoPtr->GetServiceSettingsPath();
				}
				else if(informationId == "Arguments"){
					elementInformation = serviceInfoPtr->GetServiceArguments().join(' ');
				}
				else if(informationId == "Type"){
					agentinodata::IServiceInfo::SettingsType settingsType  = serviceInfoPtr->GetSettingsType();
					switch (settingsType){
					case agentinodata::IServiceInfo::ST_PLUGIN:
						elementInformation = "ACF";
						break;
					default:
						elementInformation = "None";
						break;
					}
				}
				else if(informationId == "Status" || informationId == "StatusName"){
					agentinodata::IServiceStatusInfo::ServiceStatus status = agentinodata::IServiceStatusInfo::SS_UNDEFINED;

					if (m_serviceStatusCollectionCompPtr.IsValid()){
						imtbase::IObjectCollection::DataPtr serviceStatusDataPtr;
						if (m_serviceStatusCollectionCompPtr->GetObjectData(collectionId, serviceStatusDataPtr)){
							agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(serviceStatusDataPtr.GetPtr());
							if (serviceStatusInfoPtr != nullptr){
								status = serviceStatusInfoPtr->GetServiceStatus();
							}
						}
					}

					agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(status);
					if (informationId == "Status"){
						elementInformation = processStateEnum.id;
					}
					else{
						elementInformation = processStateEnum.name;
					}
				}
				else if(informationId == "IsAutoStart"){
					elementInformation = serviceInfoPtr->IsAutoStart();
				}

				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);
			}

			return retVal;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel* CServiceCollectionControllerComp::ListObjects(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = nullptr;
	imtbase::CTreeItemModel* itemsModel = nullptr;
	imtbase::CTreeItemModel* notificationModel = nullptr;

	if (!errorMessage.isEmpty()){
		imtbase::CTreeItemModel* errorsItemModel = rootModelPtr->AddTreeModel("errors");
		errorsItemModel->SetData("message", errorMessage);
	}
	else{
		dataModel = new imtbase::CTreeItemModel();
		itemsModel = new imtbase::CTreeItemModel();
		notificationModel = new imtbase::CTreeItemModel();

		QByteArray agentId;
		const imtgql::CGqlObject* viewParamsGql = nullptr;
		if (inputParams.size() > 0){
			viewParamsGql = inputParams.at(0).GetFieldArgumentObjectPtr("viewParams");

			const imtgql::CGqlObject* addition = inputParams.at(0).GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
			}
		}

		imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			}
		}

		if (serviceCollectionPtr == nullptr){
			errorMessage = QString("Unable to get list objects. Internal error.");
			SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

			return nullptr;
		}

		iprm::CParamsSet filterParams;

		int offset = 0, count = -1;

		if (viewParamsGql != nullptr){
			offset = viewParamsGql->GetFieldArgumentValue("Offset").toInt();
			count = viewParamsGql->GetFieldArgumentValue("Count").toInt();
			PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
		}

		int elementsCount = serviceCollectionPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

		imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds(offset, count, &filterParams);

		for (QByteArray id: ids){
			int itemIndex = itemsModel->InsertNewItem();
			if (itemIndex >= 0){
				if (!SetupGqlItem(gqlRequest, *itemsModel, itemIndex, id, errorMessage)){
					SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

					return nullptr;
				}
			}
		}
		itemsModel->SetIsArray(true);
		dataModel->SetExternTreeModel("items", itemsModel);
		dataModel->SetExternTreeModel("notification", notificationModel);
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CServiceCollectionControllerComp::GetMetaInfo(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	QByteArray agentId;
	QByteArray serviceId;
	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		serviceId = gqlInputParamPtr->GetFieldArgumentValue("Id").toByteArray();

		const imtgql::CGqlObject* addition =gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
		}
	}

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		return nullptr;
	}

	imtbase::IObjectCollection* dependantCollectionPtr = nullptr;
	imtbase::IObjectCollection* inputCollectionPtr = nullptr;

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
		agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			dependantCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
			inputCollectionPtr = serviceInfoPtr->GetInputConnections();
		}
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel);
	imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");

	if (inputCollectionPtr != nullptr){
		int index = dataModelPtr->InsertNewItem();

		dataModelPtr->SetData("Name", "Incoming Connections", index);

		imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);

		imtbase::ICollectionInfo::Ids connectionIds = inputCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (inputCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (urlConnectionParamPtr != nullptr){
					int childIndex = contentModelPtr->InsertNewItem();
					QUrl url = urlConnectionParamPtr->GetUrl();
					QByteArray usageId = urlConnectionParamPtr->GetUsageId();

					contentModelPtr->SetData("Value", usageId + " (Port: " + QString::number(url.port()) + ")", childIndex);
				}
			}
		}
	}

	if (dependantCollectionPtr != nullptr){
		int index = dataModelPtr->InsertNewItem();

		dataModelPtr->SetData("Name", "Dependant Connections", index);

		imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);

		imtbase::ICollectionInfo::Ids connectionIds = dependantCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (dependantCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* urlConnectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (urlConnectionLinkParamPtr != nullptr){
					int childIndex = contentModelPtr->InsertNewItem();
					QByteArray usageId = urlConnectionLinkParamPtr->GetUsageId();

					contentModelPtr->SetData("Value", usageId, childIndex);
				}
			}
		}
	}

	return rootModelPtr.PopPtr();
}


} // namespace agentinogql


