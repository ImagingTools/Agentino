#include <agentinogql/CTopologyControllerComp.h>


//Qt includes
#include <QtCore/QPoint>

// ACF includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// reimplemented (sdl::agentino::Topology::CGraphQlHandlerCompBase)

sdl::agentino::Topology::CTopology CTopologyControllerComp::OnGetTopology(
			const sdl::agentino::Topology::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Topology::CTopology response;
	
	response.Version_1_0.emplace();
	
	QList<sdl::agentino::Topology::CService::V1_0> services;
		
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
				
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				Q_ASSERT (serviceCollectionPtr != nullptr);
				
				imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
				for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
					agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
					imtbase::IObjectCollection::DataPtr serviceDataPtr;
					if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
						serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					}
					
					if (serviceInfoPtr == nullptr){
						continue;
					}
					
					sdl::agentino::Topology::CService::V1_0 service;
					
					service.Id = serviceElementId;
					service.AgentId = elementId;
					
					QPoint point = GetServiceCoordinate(serviceElementId);
					
					service.X = point.x();
					service.Y = point.y();
					
					QString serviceTypeName = serviceInfoPtr->GetServiceTypeName();
					QString serviceName = serviceInfoPtr->GetServiceName();
					QString description = serviceInfoPtr->GetServiceDescription();

					service.MainText = serviceName + "@" + agentName;
					service.SecondText = description;
					
					QString typeName = serviceInfoPtr->GetServiceTypeName();
					service.ThirdText = serviceTypeName;
					
					agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceElementId);
					if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
						service.Status = sdl::agentino::Topology::ServiceStatus::RUNNING;
						service.Icon1 = "Icons/Running";
					}
					else if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
						service.Status = sdl::agentino::Topology::ServiceStatus::NOT_RUNNING;
						service.Icon1 = "Icons/Stopped";
					}
					else{
						service.Status = sdl::agentino::Topology::ServiceStatus::UNDEFINED;
						service.Icon1 = "Icons/Alert";
					}
					
					agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequiredServices = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceElementId);
					if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
						|| serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED
						|| stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_RUNNING){
						service.Icon2 = "";
					}
					else if (stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING){
						service.Icon2 = "Icons/Error";
					}
					else if (stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_UNDEFINED){
						service.Icon2 = "Icons/Warning";
					}

					QList<sdl::agentino::Topology::CLink::V1_0> linkList;
					
					// Get Connections
					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
					if (connectionCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
						for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
							imtbase::IObjectCollection::DataPtr connectionDataPtr;
							if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
								imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
								if (connectionLinkParamPtr != nullptr){
									sdl::agentino::Topology::CLink::V1_0 linkData;
									QByteArray serviceId =  m_serviceCompositeInfoCompPtr->GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());

									linkData.Id = serviceId;
									
									linkList << linkData;
								}
							}
						}
					}
					
					service.Links = linkList;
					
					services << service;
				}
			}
		}
	}
	
	response.Version_1_0->Services = services;
	
	return response;
}


sdl::agentino::Topology::CSaveTopologyResponse CTopologyControllerComp::OnSaveTopology(
			const sdl::agentino::Topology::CSaveTopologyGqlRequest& saveTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Topology::CSaveTopologyResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->Successful = false;
	
	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}
		
	sdl::agentino::Topology::SaveTopologyRequestArguments arguments = saveTopologyRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		errorMessage = QString("Unable to save topology. Error: GraphQL version 1.0 is invalid");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
		return response;
	}
	
	m_topologyCollectionCompPtr->ResetData();
	
	QList<sdl::agentino::Topology::CService::V1_0> serviceList = *arguments.input.Version_1_0->Services;
	for (const sdl::agentino::Topology::CService::V1_0& service : serviceList){
		int x = *service.X;
		int y = *service.Y;
		QByteArray id = *service.Id;
		QPoint point(x,y);
		
		if (!SetServiceCoordinate(id, point)){
			response.Version_1_0->Successful = false;
			return response;
		}
	}
	
	response.Version_1_0->Successful = true;
	
	return response;
}


QPoint CTopologyControllerComp::GetServiceCoordinate(const QByteArray& serviceId) const
{
	QPoint retVal;
	
	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
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
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return false;
	}
	
	i2d::CPosition2d position2d;
	i2d::CVector2d position;
	
	position.SetX(point.x());
	position.SetY(point.y());
	position2d.SetPosition(position);
	
	QByteArray retVal = m_topologyCollectionCompPtr->InsertNewObject("Topology", "", "", &position2d, serviceId);
	return !retVal.isEmpty();
}


} // namespace agentinogql


