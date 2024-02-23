#include <agentinogql/CTopologyControllerComp.h>


// Acf includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
// #include <imtgql/CGqlObject.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>

//Qt includes
#include <QtCore/QPoint>


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
	if (!m_agentCollectionCompPtr.IsValid()){
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
						if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
							agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							int index = itemsModel->InsertNewItem();
							QPoint point = GetServiceCoordinate(serviceElementId);
							itemsModel->SetData("Id", serviceElementId, index);
							itemsModel->SetData("X", point.x(), index);
							itemsModel->SetData("Y", point.y(), index);
							QString name = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::ICollectionInfo::EIT_NAME).toString();
							QString description = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
							itemsModel->SetData("MainText", agentName + "@" + name, index);
							itemsModel->SetData("SecondText", description, index);

							if (serviceInfoPtr != nullptr){
								// Get Connections
								imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetConnectionCollection();
								if (connectionCollectionPtr != nullptr){
									imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
									for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
										qDebug() << "connectionElementId" << connectionElementId;
										imtbase::IObjectCollection::DataPtr connectionDataPtr;
										if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
											imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
											if (connectionParamPtr != nullptr){
												if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_OUTPUT){
													QUrl url = connectionParamPtr->GetUrl();
													QString connectionServiceName = connectionParamPtr->GetServiceName();
													QByteArray serviceId =  GetServiceId(url, connectionServiceName);
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


QByteArray CTopologyControllerComp::GetServiceId(const QUrl& url, const QString& connectionServiceName) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
							agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr != nullptr){
								// Get Connections
								imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetConnectionCollection();
								if (connectionCollectionPtr != nullptr){
									imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
									for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
										qDebug() << "connectionElementId" << connectionElementId;
										imtbase::IObjectCollection::DataPtr connectionDataPtr;
										if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
											imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
											if (connectionParamPtr != nullptr){
												if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
													qDebug() << connectionParamPtr->GetUrl();
													QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
													for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
														qDebug() << incomingConnection.url;
														if (/*connectionParamPtr->() == connectionServiceName && */incomingConnection.url == url){
															return serviceElementId;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return QByteArray();
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


