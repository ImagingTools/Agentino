#include <agentinogql/CTopologyControllerComp.h>


//Qt includes
#include <QtCore/QPoint>

// Acf includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>



namespace agentinogql
{


// reimplemented (imtgql::CGqlRequestHandlerCompBase)

imtbase::CTreeItemModel* CTopologyControllerComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	imtgql::IGqlRequest::RequestType requestType = gqlRequest.GetRequestType();

	if (requestType == imtgql::IGqlRequest::RT_QUERY){
		if (commandId == "GetTopology"){
			return CreateTopologyModel();
		}
	}
	else{
		if (commandId == "SaveTopology"){
			return SaveTopologyModel(gqlRequest, errorMessage);
		}
	}

	errorMessage = QString("Unable to create internal response with command %1").arg(qPrintable(commandId));

	SendErrorMessage(0, errorMessage);

	Q_ASSERT(false);

	return nullptr;
}


imtbase::CTreeItemModel* CTopologyControllerComp::CreateTopologyModel() const
{
	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* representationPtr = rootModelPtr->AddTreeModel("data");
	Q_ASSERT(representationPtr != nullptr);
	imtbase::CTreeItemModel* itemsModel = representationPtr->AddTreeModel("items");
	Q_ASSERT(itemsModel != nullptr);

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr);
						agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
						if (serviceInfoPtr == nullptr){
							continue;
						}
						int index = itemsModel->InsertNewItem();
						QPoint point = GetServiceCoordinate(serviceElementId);
						itemsModel->SetData("Id", serviceElementId, index);
						itemsModel->SetData("AgentId", elementId, index);
						itemsModel->SetData("X", point.x(), index);
						itemsModel->SetData("Y", point.y(), index);
						QString name = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::ICollectionInfo::EIT_NAME).toString();
						QString description = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
						QString typeName = serviceInfoPtr->GetServiceTypeName();
						itemsModel->SetData("MainText", name + "@" + agentName, index);
						itemsModel->SetData("SecondText", description, index);
						itemsModel->SetData("ThirdText", typeName, index);
						QString serviceStatus = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceElementId);
						itemsModel->SetData("Status", serviceStatus, index);
						if (serviceStatus == "Running"){
							itemsModel->SetData("IconUrl_1", "Icons/Running", index);
						}
						else if (serviceStatus == "NotRunning"){
							itemsModel->SetData("IconUrl_1", "Icons/Stopped", index);
						}
						else{
							itemsModel->SetData("IconUrl_1", "Icons/Alert", index);
						}

						QString dependantServiceStatus = m_serviceCompositeInfoCompPtr->GetDependantServiceStatus(serviceElementId);
						itemsModel->SetData("DependantStatus", dependantServiceStatus, index);

						if (dependantServiceStatus == agentino::DependencyStatus::s_NotAllRunning){
							itemsModel->SetData("IconUrl_2", "Icons/Error", index);
						}
						else if (dependantServiceStatus == agentino::DependencyStatus::s_Undefined){
							itemsModel->SetData("IconUrl_2", "Icons/Warning", index);
						}
						else {
							itemsModel->SetData("IconUrl_2", "", index);
						}

						// Get Connections
						imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
						if (connectionCollectionPtr != nullptr){
							imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
							for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
								imtbase::IObjectCollection::DataPtr connectionDataPtr;
								if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
									imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
									if (connectionLinkParamPtr != nullptr){
										QByteArray serviceId =  m_serviceCompositeInfoCompPtr->GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
										// itemsModel->SetData("SecondText", description, index);
										imtbase::CTreeItemModel* linkModel = itemsModel->GetTreeItemModel("Links", index);
										if (linkModel == nullptr){
											linkModel = itemsModel->AddTreeModel("Links", index);
										}
										int linkIndex = linkModel->InsertNewItem();
										linkModel->SetData("ObjectId", serviceId, linkIndex);
									}
								}
							}

						}
					}
				}
			}
		}
	}

	return rootModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CTopologyControllerComp::SaveTopologyModel(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	const imtgql::CGqlObject* inputParams = gqlRequest.GetParam("input");
	if (inputParams == nullptr || !m_topologyCollectionCompPtr.IsValid()){
		return nullptr;
	}
	QByteArray itemData = inputParams->GetFieldArgumentValue("Item").toByteArray();
	if (!itemData.isEmpty()){
		imtbase::CTreeItemModel itemModel;
		itemModel.CreateFromJson(itemData);

		m_topologyCollectionCompPtr->ResetData();
		for (int index = 0; index < itemModel.GetItemsCount(); index++){
			int x = itemModel.GetData("X", index).toInt();
			int y = itemModel.GetData("Y", index).toInt();
			QByteArray id = itemModel.GetData("Id", index).toByteArray();
			QPoint point(x,y);
			SetServiceCoordinate(id, point);
		}

		istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
		imtbase::CTreeItemModel* representationPtr = rootModelPtr->AddTreeModel("data");
		Q_ASSERT(representationPtr != nullptr);
		imtbase::CTreeItemModel* notificationModel = representationPtr->AddTreeModel("notification");
		Q_ASSERT(notificationModel != nullptr);
		notificationModel->SetData("successful", "true");
		return rootModelPtr.PopPtr();
	}
	return nullptr;
}


QPoint CTopologyControllerComp::GetServiceCoordinate(const QByteArray& serviceId) const
{
	QPoint retVal;

	if (!m_topologyCollectionCompPtr.IsValid()){
		return retVal;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_topologyCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		i2d::CPosition2d* position2dPtr = dynamic_cast<i2d::CPosition2d*>(dataPtr.GetPtr());
		if (position2dPtr != nullptr){
			i2d::CVector2d position = position2dPtr->GetPosition();
			retVal.setX(position.GetX());
			retVal.setY(position.GetY());
		}
	}

	return retVal;
}


bool CTopologyControllerComp::SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const
{
	if (!m_topologyCollectionCompPtr.IsValid()){
		return false;
	}

	i2d::CPosition2d position2d;
	i2d::CVector2d position;

	position.SetX(point.x());
	position.SetY(point.y());
	position2d.SetPosition(position);

	m_topologyCollectionCompPtr->InsertNewObject("Topology", "", "", &position2d, serviceId);

	return true;
}


} // namespace agentinogql


