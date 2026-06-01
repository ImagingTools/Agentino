// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


//Qt includes
#include <QtCore/QPoint>

// ACF includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// reimplemented (icomp::CComponentBase)

void CTopologyControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	QByteArray featureFlag = qgetenv("AGENTINO_TOPOLOGY_SNAPSHOT_AGGREGATOR");
	m_useAggregation = (featureFlag == "1" || featureFlag == "true" || featureFlag == "on");
}


// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologyControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	if (m_useAggregation){
		return GetAggregatedTopology(gqlRequest);
	}
	return GetLegacyTopology();
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

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> serviceList = *arguments.input->services;
	for (const istd::TNullableValue<sdl::V1_0::agentino::CService>& service : *serviceList.GetPtr()){
		int x = *service->x;
		int y = *service->y;
		QByteArray id = *service->id;
		QPoint point(x,y);

		if (!SetServiceCoordinate(id, point)){
			response.successful = false;
			return response;
		}
	}
	
	response.successful = true;
	
	return response;
}


// private methods

sdl::V1_0::agentino::CTopology CTopologyControllerComp::GetLegacyTopology() const
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
					
					QPoint point = GetServiceCoordinate(serviceElementId);
					
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


sdl::V1_0::agentino::CTopology CTopologyControllerComp::GetAggregatedTopology(
			const ::imtgql::CGqlRequest& gqlRequest) const
{
	sdl::V1_0::agentino::CTopology response;
	QList<sdl::V1_0::agentino::CService> allServices;

	// Collect the current set of agent IDs
	imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();

	// Clean up cache entries for agents no longer in the collection
	{
		QMutexLocker locker(&m_snapshotMutex);
		QList<QByteArray> cachedAgentIds = m_snapshotCache.keys();
		for (const QByteArray& cachedId: cachedAgentIds){
			if (!agentIds.contains(cachedId)){
				m_snapshotCache.remove(cachedId);
			}
		}
	}

	for (const imtbase::ICollectionInfo::Id& agentId: agentIds){
		sdl::V1_0::agentino::CTopology agentSnapshot;
		bool isFresh = GetAgentSnapshot(agentId, gqlRequest, agentSnapshot);

		if (!isFresh){
			// Use cached snapshot if available, marking services as stale
			QMutexLocker locker(&m_snapshotMutex);
			if (m_snapshotCache.contains(agentId)){
				agentSnapshot = m_snapshotCache[agentId].snapshot;
				MarkServicesAsStale(agentSnapshot);
			}
			else{
				continue;
			}
		}
		else{
			// Update cache with fresh snapshot
			QMutexLocker locker(&m_snapshotMutex);
			SnapshotInfo info;
			info.snapshot = agentSnapshot;
			info.lastSnapshotAt = QDateTime::currentDateTime();
			m_snapshotCache[agentId] = info;
		}

		// Set agentId on all services and collect them
		if (agentSnapshot.services.has_value()){
			QString agentName = m_agentCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
			for (const istd::TNullableValue<sdl::V1_0::agentino::CService>& serviceNullable: *agentSnapshot.services.GetPtr()){
				if (serviceNullable.has_value()){
					sdl::V1_0::agentino::CService service = *serviceNullable;
					service.agentId = agentId;

					// Prepend agent name to mainText for display (e.g. "ServiceName@AgentName")
					if (service.mainText.has_value()){
						service.mainText = *service.mainText + "@" + agentName;
					}

					allServices << service;
				}
			}
		}
	}

	response.services.Emplace();
	response.services->FromList(allServices);

	// Apply centrally-stored layout coordinates
	ApplyLayoutCoordinates(response);

	return response;
}


bool CTopologyControllerComp::GetAgentSnapshot(
			const QByteArray& agentId,
			const ::imtgql::CGqlRequest& gqlRequest,
			sdl::V1_0::agentino::CTopology& snapshot) const
{
	// Clone the incoming request and inject the agent's clientid header
	imtgql::CGqlRequest* gqlRequestPtr = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_QUERY, "GetTopology");

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	gqlContextPtr->SetContextParam("clientid", agentId);
	gqlRequestPtr->SetGqlContext(gqlContextPtr);

	imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlRequestPtr);

	sdl::V1_0::agentino::CTopology result;
	bool success = SendModelRequest<sdl::V1_0::agentino::CTopology>(requestPtr, result);

	if (success){
		snapshot = result;
		return true;
	}

	return false;
}


void CTopologyControllerComp::ApplyLayoutCoordinates(sdl::V1_0::agentino::CTopology& topology) const
{
	if (!topology.services.has_value() || !m_topologyCollectionCompPtr.IsValid()){
		return;
	}

	for (istd::TNullableValue<sdl::V1_0::agentino::CService>& serviceNullable: *topology.services.GetPtr()){
		if (serviceNullable.has_value() && serviceNullable->id.has_value()){
			QPoint point = GetServiceCoordinate(*serviceNullable->id);
			serviceNullable->x = point.x();
			serviceNullable->y = point.y();
		}
	}
}


void CTopologyControllerComp::MarkServicesAsStale(sdl::V1_0::agentino::CTopology& topology) const
{
	if (!topology.services.has_value()){
		return;
	}

	for (istd::TNullableValue<sdl::V1_0::agentino::CService>& serviceNullable: *topology.services.GetPtr()){
		if (serviceNullable.has_value()){
			serviceNullable->status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
			serviceNullable->icon1 = "Icons/Alert";
			serviceNullable->icon2 = "Icons/Warning";
			serviceNullable->hasError = true;
		}
	}
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


