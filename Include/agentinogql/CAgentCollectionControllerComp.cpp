#include <agentinogql/CAgentCollectionControllerComp.h>


// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
//#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>

// ServiceManager includes
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
					elementInformation = agentPtr->GetAgentName();
				}
				else if(informationId == "Description"){
					elementInformation = agentPtr->GetAgentDescription();
				}
				else if(informationId == "HttpUrl"){
					elementInformation = agentPtr->GetHttpUrl();
				}
				else if(informationId == "WebSocketUrl"){
					elementInformation = agentPtr->GetWebSocketUrl();
				}
				else if(informationId == "LastConnection"){
					QDateTime lastConnection = agentPtr->GetLastConnection();
					elementInformation = lastConnection.toString("dd.MM.yyyy");
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


imtbase::CTreeItemModel *CAgentCollectionControllerComp::ListObjects(const imtgql::CGqlRequest &gqlRequest, QString &errorMessage) const
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

	QByteArray accountId = GetObjectIdFromInputParams(gqlRequest.GetParams());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(accountId, dataPtr)){
		const agentinodata::CAgentInfo* agentPtr = dynamic_cast<const agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentPtr != nullptr){
			QString name = agentPtr->GetAgentName();
			QString description = agentPtr->GetAgentDescription();
			QString httpUrl = agentPtr->GetHttpUrl();
			QString webSocketUrl = agentPtr->GetWebSocketUrl();
			QDateTime lastConnection = agentPtr->GetLastConnection();

			dataModel->SetData("Id", accountId);
			dataModel->SetData("Name", name);
			dataModel->SetData("Description", description);
			dataModel->SetData("HttpUrl", httpUrl);
			dataModel->SetData("WebSocketUrl", webSocketUrl);
			dataModel->SetData("LastConnection", lastConnection.toString("dd.MM.yyyy"));
		}
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


istd::IChangeable* CAgentCollectionControllerComp::CreateObject(
		const QList<imtgql::CGqlObject>& inputParams,
		QByteArray &objectId,
		QString &name,
		QString &description,
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

		agentPtr->SetAgentName(name.toUtf8());

		if (itemModel.ContainsKey("Description")){
			description = itemModel.GetData("Description").toString();
			agentPtr->SetAgentDescription(description.toUtf8());
		}

		if (itemModel.ContainsKey("HttpUrl")){
			QByteArray httpUrl = itemModel.GetData("HttpUrl").toByteArray();
			agentPtr->SetHttpUrl(httpUrl);
		}

		if (itemModel.ContainsKey("WebSocketUrl")){
			QByteArray webSocketUrl = itemModel.GetData("WebSocketUrl").toByteArray();
			agentPtr->SetWebSocketUrl(webSocketUrl);
		}

//		if (itemModel.ContainsKey("LastConnection")){
//			QByteArray lastConnection = itemModel.GetData("LastConnection").toByteArray();
//			agentPtr->SetLastConnection(QDateTime::fromString(lastConnection, "dd.MM.yyyy"));
//		}

		return agentPtr;
	}

	errorMessage = QObject::tr("Can not create agent: %1").arg(QString(objectId));

	return nullptr;
}


imtbase::CTreeItemModel* CAgentCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

//	QByteArray oldObjectId;
//	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");
//	if (inputParamPtr != nullptr){
//		oldObjectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();
//	}

	QByteArray objectId = GetObjectIdFromInputParams(inputParams);
	const istd::IChangeable* agentObject = m_objectCollectionCompPtr->GetObjectPtr(objectId);
	if (agentObject == nullptr){
		return BaseClass::InsertObject(gqlRequest, errorMessage);
	}
	else {
		return BaseClass::UpdateObject(gqlRequest, errorMessage);
	}
}


} // namespace agentinogql


