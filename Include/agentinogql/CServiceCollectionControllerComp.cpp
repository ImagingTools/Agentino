#include <agentinogql/CServiceCollectionControllerComp.h>


// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>

// ServiceManager includes
#include <agentinodata/CRepresentationService.h>


namespace agentinogql
{


bool CServiceCollectionControllerComp::SetupGqlItem(
		const imtgql::CGqlRequest& gqlRequest,
		imtbase::CTreeItemModel& model,
		int itemIndex,
		const QByteArray& collectionId,
		QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty() && m_objectCollectionCompPtr.IsValid()){
		agentinodata::CIdentifiableRepresentationService* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(collectionId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableRepresentationService*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr){
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = m_objectCollectionCompPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "Id"){
					elementInformation = serviceInfoPtr->GetObjectUuid();
				}
				else if(informationId == "Name"){
					elementInformation = serviceInfoPtr->GetServiceName();
				}
				else if(informationId == "Description"){
					elementInformation = serviceInfoPtr->GetServiceDescription();
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
					agentinodata::IServiceInfo::ServiceType serviceType  = serviceInfoPtr->GetServiceType();
					switch (serviceType){
					case agentinodata::IServiceInfo::ST_ACF:
						elementInformation = "ACF";
						break;
					default:
						elementInformation = "None";
						break;
					}
				}


				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);

				if(informationId == "Point"){
					QPointF point = serviceInfoPtr->GetPoint();
					imtbase::CTreeItemModel* pointModel = model.AddTreeModel("Point");
					elementInformation = QByteArray::number(point.x());
					retVal = retVal && pointModel->SetData("X", elementInformation, itemIndex);
					elementInformation = QByteArray::number(point.y());
					retVal = retVal && pointModel->SetData("Y", elementInformation, itemIndex);
				}
			}

			return true;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel *CServiceCollectionControllerComp::ListObjects(const imtgql::CGqlRequest &gqlRequest, QString &errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return nullptr;
	}

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

		const imtgql::CGqlObject* viewParamsGql = nullptr;
		if (inputParams.size() > 0){
			viewParamsGql = inputParams.at(0).GetFieldArgumentObjectPtr("viewParams");
		}

		iprm::CParamsSet filterParams;

		int offset = 0, count = -1;

		if (viewParamsGql != nullptr){
			offset = viewParamsGql->GetFieldArgumentValue("Offset").toInt();
			count = viewParamsGql->GetFieldArgumentValue("Count").toInt();
			PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
		}

		int elementsCount = m_objectCollectionCompPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

		imtbase::ICollectionInfo::Ids ids = m_objectCollectionCompPtr->GetElementIds(offset, count, &filterParams);

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


imtbase::CTreeItemModel* CServiceCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QObject::tr("Internal error").toUtf8();
		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();

	QByteArray accountId = GetObjectIdFromInputParams(gqlRequest.GetParams());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(accountId, dataPtr)){
		const agentinodata::CIdentifiableRepresentationService* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableRepresentationService*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QString name = serviceInfoPtr->GetServiceName();
			QString description = serviceInfoPtr->GetServiceDescription();
			QString path = serviceInfoPtr->GetServicePath();
			QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
			QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');

			dataModel->SetData("Id", accountId);
			dataModel->SetData("Name", name);
			dataModel->SetData("Description", description);
			dataModel->SetData("Path", path);
			dataModel->SetData("SettingsPath", settingsPath);
			dataModel->SetData("Arguments", arguments);
			agentinodata::IServiceInfo::ServiceType serviceType  = serviceInfoPtr->GetServiceType();
			switch (serviceType){
			case agentinodata::IServiceInfo::ST_ACF:
				dataModel->SetData("Type", "ACF");
				break;
			default:
				dataModel->SetData("Type", "None");
				break;
			}
		}
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


istd::IChangeable* CServiceCollectionControllerComp::CreateObject(
		const QList<imtgql::CGqlObject>& inputParams,
		QByteArray &objectId,
		QString &name,
		QString &description,
		QString& errorMessage) const
{
	if (!m_serviceInfoFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	objectId = GetObjectIdFromInputParams(inputParams);
	if (objectId.isEmpty()){
		objectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}

	QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
	if (!itemData.isEmpty()){
		agentinodata::IServiceInfo* serviceInstancePtr = m_serviceInfoFactCompPtr.CreateInstance();
		if (serviceInstancePtr == nullptr){
			return nullptr;
		}

		agentinodata::CIdentifiableRepresentationService* serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableRepresentationService*>(serviceInstancePtr);
		if (serviceInfoPtr == nullptr){
			errorMessage = QT_TR_NOOP("Unable to get an service info!");
			return nullptr;
		}

		imtbase::CTreeItemModel itemModel;
		itemModel.CreateFromJson(itemData);

		serviceInfoPtr->SetObjectUuid(objectId);

		if (itemModel.ContainsKey("Name")){
			name = itemModel.GetData("Name").toString();
		}

		if (name.isEmpty()){
			errorMessage = QT_TR_NOOP("Service name can't be empty");
			return nullptr;
		}

		serviceInfoPtr->SetServiceName(name.toUtf8());

		if (itemModel.ContainsKey("Description")){
			description = itemModel.GetData("Description").toString();
			serviceInfoPtr->SetServiceDescription(description.toUtf8());
		}

		if (itemModel.ContainsKey("Path")){
			QByteArray path = itemModel.GetData("Path").toByteArray();
			serviceInfoPtr->SetServicePath(path);
		}

		if (itemModel.ContainsKey("SettingsPath")){
			QByteArray settingsPath = itemModel.GetData("SettingsPath").toByteArray();
			serviceInfoPtr->SetServiceSettingsPath(settingsPath);
		}

		if (itemModel.ContainsKey("Arguments")){
			QByteArray arguments = itemModel.GetData("Arguments").toByteArray();
			serviceInfoPtr->SetServiceArguments(arguments.split(' '));
		}

		return serviceInfoPtr;
	}

	errorMessage = QObject::tr("Can not create service: %1").arg(QString(objectId));

	return nullptr;
}





} // namespace agentinogql


