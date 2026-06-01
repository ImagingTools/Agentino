// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology.h>


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


// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologyControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;
	
	
	QList<sdl::V1_0::agentino::CService> services;
		
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();

				// Prefer agent-local topology positions when the proxy is available.
				// Fall back to the central topology collection if the agent is offline.
				QMap<QByteArray, QPoint> agentPositions;
				if (m_agentTopologyProxyCompPtr.IsValid()){
					QString proxyError;
					const sdl::V1_0::agentino::CLocalTopology agentTopology =
							m_agentTopologyProxyCompPtr->QueryLocalTopology(elementId, proxyError);

					if (proxyError.isEmpty() && agentTopology.positions.has_value()){
						for (const istd::TNullableValue<sdl::V1_0::agentino::CLocalServicePosition>& pos :
								*agentTopology.positions){
							if (pos.has_value() && pos->id.has_value()){
								const int x = pos->x.has_value() ? *pos->x : 0;
								const int y = pos->y.has_value() ? *pos->y : 0;
								agentPositions.insert(*pos->id, QPoint(x, y));
							}
						}
					}
				}
				
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
					
					sdl::V1_0::agentino::CService service;
					
					service.id = serviceElementId;
					service.agentId = elementId;

					// Resolve position: agent-local data takes precedence over central storage.
					QPoint point;
					if (agentPositions.contains(serviceElementId)){
						point = agentPositions.value(serviceElementId);
					}
					else{
						point = GetServiceCoordinate(serviceElementId);
					}
					
					service.x = point.x();
					service.y = point.y();
					
					QString serviceTypeName = serviceInfoPtr->GetServiceTypeId();
					QString serviceName = serviceInfoPtr->GetServiceName();
					QString description = serviceInfoPtr->GetServiceDescription();

					service.mainText = serviceName + "@" + agentName;
					service.secondText = description;
					
					service.thirdText = serviceTypeName;
					
					agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceElementId);
					if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
						service.status = sdl::V1_0::agentino::ServiceStatus::RUNNING;
						service.icon1 = "Icons/Running";
					}
					else if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
						service.status = sdl::V1_0::agentino::ServiceStatus::NOT_RUNNING;
						service.icon1 = "Icons/Stopped";
					}
					else{
						service.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
						service.icon1 = "Icons/Alert";
					}
					
					agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequiredServices = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceElementId);
					if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
						|| serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED
						|| stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_RUNNING){
						service.icon2 = "";
					}
					else if (stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING){
						service.icon2 = "Icons/Error";
					}
					else if (stateOfRequiredServices == agentinodata::IServiceCompositeInfo::SORS_UNDEFINED){
						service.icon2 = "Icons/Warning";
					}

					QList<sdl::V1_0::agentino::CLink> linkList;
					
					// Get Connections
					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
					if (connectionCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
						for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
							imtbase::IObjectCollection::DataPtr connectionDataPtr;
							if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
								imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
								if (connectionLinkParamPtr != nullptr){
									sdl::V1_0::agentino::CLink linkData;
									QByteArray dependantConnectionId = connectionLinkParamPtr->GetDependantServiceConnectionId();
									QByteArray serviceId =  m_serviceCompositeInfoCompPtr->GetServiceId(dependantConnectionId);

									linkData.id = serviceId;
									
									linkList << linkData;
								}
							}
						}
					}
					
					service.links.Emplace();
					service.links->FromList(linkList);
					
					services << service;
				}
			}
		}
	}

	response.services.Emplace();
	response.services->FromList(services);

	return response;
}


sdl::V1_0::agentino::CSaveTopologyResponse CTopologyControllerComp::OnSaveTopology(
			const sdl::V1_0::agentino::CSaveTopologyGqlRequest& saveTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSaveTopologyResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::SaveTopologyRequestArguments arguments = saveTopologyRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to save topology. Error: GraphQL version 1.0 is invalid");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	m_topologyCollectionCompPtr->ResetData();
	if (!arguments.input->services.has_value()){
		errorMessage = QString("Unable to save topology. Error: Input params is invalid");
		return response;
	}

	// Group positions by agentId for agent-local push.
	QMap<QByteArray, sdl::V1_0::agentino::CLocalTopology> agentTopologies;

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> serviceList = *arguments.input->services;
	for (const istd::TNullableValue<sdl::V1_0::agentino::CService>& service : *serviceList.GetPtr()){
		int x = *service->x;
		int y = *service->y;
		QByteArray id = *service->id;
		QPoint point(x,y);

		// Always update the central topology collection for backward compatibility.
		if (!SetServiceCoordinate(id, point)){
			response.successful = false;
			return response;
		}

		// Accumulate agent-local topology updates when the proxy is configured.
		if (m_agentTopologyProxyCompPtr.IsValid() && service->agentId.has_value()){
			const QByteArray agentId = *service->agentId;

			sdl::V1_0::agentino::CLocalServicePosition position;
			position.id = id;
			position.x = x;
			position.y = y;

			sdl::V1_0::agentino::CLocalTopology& agentTopology = agentTopologies[agentId];
			if (!agentTopology.positions.has_value()){
				agentTopology.positions.Emplace();
			}
			agentTopology.positions->Append(position);
		}
	}

	// Push positions to each agent so the agent becomes the source of truth.
	if (m_agentTopologyProxyCompPtr.IsValid()){
		for (auto it = agentTopologies.constBegin(); it != agentTopologies.constEnd(); ++it){
			QString pushError;
			if (!m_agentTopologyProxyCompPtr->PushLocalTopology(it.key(), it.value(), pushError)){
				// Non-fatal: agent may be temporarily offline; central storage is already updated.
				SendErrorMessage(0,
					QString("Unable to push topology to agent '%1'. Error: %2").arg(qPrintable(it.key()), pushError),
					"CTopologyControllerComp");
			}
		}
	}
	
	response.successful = true;
	
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


