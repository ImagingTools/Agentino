#include <agentinogql/CAgentCollectionControllerComp.h>


// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentinogql
{


bool CAgentCollectionControllerComp::SetupGqlItem(
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
		agentinodata::CIdentifiableAgentInfo* agentPtr = nullptr;
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(collectionId, agentDataPtr)){
			agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentDataPtr.GetPtr());
		}

		if (agentPtr != nullptr){
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "Id"){
					elementInformation = agentPtr->GetObjectUuid();
				}
				else if(informationId == "Name"){
					QString name = m_objectCollectionCompPtr->GetElementInfo(collectionId, imtbase::ICollectionInfo::EIT_NAME).toString();
					elementInformation = name;
				}
				else if(informationId == "Description"){
					QString description = m_objectCollectionCompPtr->GetElementInfo(collectionId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();

					elementInformation = description;
				}
				else if(informationId == "ComputerName"){
					elementInformation = agentPtr->GetComputerName();
				}
				else if(informationId == "Services"){
					imtgql::CGqlRequest request(imtgql::IGqlRequest::RT_QUERY, "ServicesList");

					imtgql::CGqlObject inputObject("input");

					imtgql::CGqlObject additionObject("addition");
					additionObject.InsertField("clientId", QVariant(agentPtr->GetObjectUuid()));

					inputObject.InsertField("addition", additionObject);

					request.AddParam(inputObject);

					imtgql::CGqlObject itemsObject("items");
					itemsObject.InsertField("Id");
					itemsObject.InsertField("Name");
					itemsObject.InsertField("Description");
					request.AddField(itemsObject);

					if (m_requestHandlerPtr.IsValid()){
						imtbase::CTreeItemModel* responseModelPtr = m_requestHandlerPtr->CreateResponse(request, errorMessage);
						if (responseModelPtr != nullptr){
							imtbase::CTreeItemModel* dataModelPtr = responseModelPtr->GetTreeItemModel("data");
							if (dataModelPtr != nullptr){
								imtbase::CTreeItemModel* itemsModelPtr = dataModelPtr->GetTreeItemModel("items");
								if (itemsModelPtr != nullptr){
									QStringList result;
									for (int i = 0; i < itemsModelPtr->GetItemsCount(); i++){
										QString name = itemsModelPtr->GetData("Name", i).toString();

										result << name;
									}

									elementInformation = result.join(';');
								}
							}
						}
					}
				}
				else if(informationId == "LastConnection"){
					QDateTime lastConnection = agentPtr->GetLastConnection();
					if (!lastConnection.isNull()){
						lastConnection.setTimeSpec(Qt::UTC);

						elementInformation = lastConnection.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
					}
				}

				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);
			}

			return true;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel *CAgentCollectionControllerComp::ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
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


imtbase::CTreeItemModel* CAgentCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QObject::tr("Internal error").toUtf8();
		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();

	QByteArray objectId = GetObjectIdFromInputParams(gqlRequest.GetParams());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(objectId, dataPtr)){
		const agentinodata::CAgentInfo* agentPtr = dynamic_cast<const agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentPtr != nullptr){
			QString name = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();
			QString description = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
			QDateTime lastConnection = agentPtr->GetLastConnection();

			dataModel->SetData("Id", objectId);
			dataModel->SetData("Name", name);
			dataModel->SetData("Description", description);
			dataModel->SetData("LastConnection", lastConnection.toString("dd.MM.yyyy"));
		}
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


istd::IChangeable* CAgentCollectionControllerComp::CreateObject(
			const QList<imtgql::CGqlObject>& inputParams,
			QByteArray& objectId,
			QString& name,
			QString& description,
			QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	objectId = GetObjectIdFromInputParams(inputParams);
	if (objectId.isEmpty()){
		objectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}

	QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
	if (!itemData.isEmpty()){
		agentinodata::IAgentInfo* agentInstancePtr = m_agentFactCompPtr.CreateInstance();
		if (agentInstancePtr == nullptr){
			return nullptr;
		}

		agentinodata::CIdentifiableAgentInfo* agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentInstancePtr);
		if (agentPtr == nullptr){
			errorMessage = QT_TR_NOOP("Unable to get an service info!");
			return nullptr;
		}

		imtbase::CTreeItemModel itemModel;
		itemModel.CreateFromJson(itemData);

		agentPtr->SetObjectUuid(objectId);

		if (itemModel.ContainsKey("Name")){
			name = itemModel.GetData("Name").toString();
		}

		if (name.isEmpty()){
			errorMessage = QT_TR_NOOP("Service name can't be empty");
			return nullptr;
		}

		if (itemModel.ContainsKey("Description")){
			description = itemModel.GetData("Description").toString();
		}

		if (itemModel.ContainsKey("ComputerName")){
			QString computerName = itemModel.GetData("ComputerName").toString();
			agentPtr->SetComputerName(computerName);
		}

		return agentPtr;
	}

	errorMessage = QObject::tr("Can not create agent: %1").arg(QString(objectId));

	return nullptr;
}


imtbase::CTreeItemModel* CAgentCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

	QByteArray objectId = GetObjectIdFromInputParams(inputParams);

	imtbase::CTreeItemModel* retVal = nullptr;
	const istd::IChangeable* agentObject = m_objectCollectionCompPtr->GetObjectPtr(objectId);
	if (agentObject == nullptr){
		// retVal = BaseClass::InsertObject(gqlRequest, errorMessage);
	}

	return retVal;
}



imtbase::CTreeItemModel* CAgentCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

	QByteArray objectId = GetObjectIdFromInputParams(inputParams);

	imtbase::CTreeItemModel* retVal = nullptr;
	const istd::IChangeable* agentObject = m_objectCollectionCompPtr->GetObjectPtr(objectId);
	if (agentObject == nullptr){
		return nullptr;
		// retVal = BaseClass::InsertObject(gqlRequest, errorMessage);
	}
	// else {
		// retVal =  BaseClass::UpdateObject(gqlRequest, errorMessage);
	// 	imtbase::IObjectCollection::DataPtr agentDataPtr;
	// 	if (m_objectCollectionCompPtr->GetObjectData(objectId, agentDataPtr)){
	// 		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
	// 		if (agentInfoPtr != nullptr){
	// 			QDateTime dateTime = QDateTime::currentDateTimeUtc();
	// 			agentInfoPtr->SetLastConnection(dateTime);

	// 			if (!m_objectCollectionCompPtr->SetObjectData(objectId, *agentInfoPtr)){
	// 				qDebug() << QString("Unable to set data to the collection object with ID: %1.").arg(qPrintable(objectId));
	// 			}
	// 		}
	// 	}
	// }



	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(objectId, agentDataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			QDateTime dateTime = QDateTime::currentDateTimeUtc();
			agentInfoPtr->SetLastConnection(dateTime);

			QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
			QString name;

			if (!itemData.isEmpty()){
				// agentinodata::IAgentInfo* agentInstancePtr = m_agentFactCompPtr.CreateInstance();
				// if (agentInstancePtr == nullptr){
				// 	return nullptr;
				// }

				// agentinodata::CIdentifiableAgentInfo* agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentInstancePtr);
				// if (agentPtr == nullptr){
				// 	errorMessage = QT_TR_NOOP("Unable to get an service info!");
				// 	return nullptr;
				// }

				imtbase::CTreeItemModel itemModel;
				itemModel.CreateFromJson(itemData);

				// agentPtr->SetObjectUuid(objectId);

				if (itemModel.ContainsKey("Name")){
					name = itemModel.GetData("Name").toString();
					m_objectCollectionCompPtr->SetElementName(objectId, name);
				}

				if (itemModel.ContainsKey("Description")){
					QString description = itemModel.GetData("Description").toString();
					m_objectCollectionCompPtr->SetElementDescription(objectId, description);
				}
			}

			if (!m_objectCollectionCompPtr->SetObjectData(objectId, *agentInfoPtr)){
				qDebug() << QString("Unable to set data to the collection object with ID: %1.").arg(qPrintable(objectId));
			}
			else{
				retVal = new imtbase::CTreeItemModel();
				imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();
				imtbase::CTreeItemModel* notificationModel = new imtbase::CTreeItemModel();
				notificationModel->SetData("Id", objectId);
				notificationModel->SetData("Name", name);
				dataModel->SetExternTreeModel("updatedNotification", notificationModel);
				retVal->SetExternTreeModel("data", dataModel);
			}
		}
	}

	return retVal;
}


} // namespace agentinogql


