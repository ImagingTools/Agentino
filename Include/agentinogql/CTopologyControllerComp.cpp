// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


//Qt includes
#include <QtCore/QPoint>

// ACF includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <istd/TInterfacePtr.h>
#include <imtgql/CGqlContext.h>
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
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	QByteArray featureFlag = qgetenv("AGENTINO_TOPOLOGY_SNAPSHOT_AGGREGATOR");
	if (featureFlag.compare("1", Qt::CaseInsensitive) == 0
		|| featureFlag.compare("true", Qt::CaseInsensitive) == 0
		|| featureFlag.compare("on", Qt::CaseInsensitive) == 0){
		return GetAggregatedTopology(gqlRequest);
	}

	return GetLegacyTopology();
}


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
	ApplyLayoutCoordinates(response);

	return response;
}


sdl::V1_0::agentino::CTopology CTopologyControllerComp::GetAggregatedTopology(const ::imtgql::CGqlRequest& gqlRequest) const
{
	sdl::V1_0::agentino::CTopology response;
	if (!m_agentCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'AgentCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	QList<sdl::V1_0::agentino::CService> services;
	imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId: agentIds){
		QString errorMessage;
		bool isFresh = false;
		sdl::V1_0::agentino::CTopology agentSnapshot = GetAgentSnapshot(gqlRequest, agentId, isFresh, errorMessage);
		if (isFresh){
			SnapshotInfo& snapshotInfo = m_snapshotInfoMap[agentId];
			snapshotInfo.snapshot = agentSnapshot;
			snapshotInfo.lastSnapshotAt = QDateTime::currentDateTimeUtc();
			snapshotInfo.isStale = false;
		}
		else if (m_snapshotInfoMap.contains(agentId)){
			SnapshotInfo& snapshotInfo = m_snapshotInfoMap[agentId];
			agentSnapshot = snapshotInfo.snapshot;
			snapshotInfo.isStale = true;
		}
		else{
			continue;
		}

		if (m_snapshotInfoMap.contains(agentId) && m_snapshotInfoMap[agentId].isStale){
			MarkServicesAsStale(agentSnapshot);
		}

		if (agentSnapshot.services.has_value()){
			istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> snapshotServices = *agentSnapshot.services;
			for (istd::TNullableValue<sdl::V1_0::agentino::CService>& service: *snapshotServices.GetPtr()){
				service->agentId = agentId;
				services << *service.GetPtr();
			}
		}
	}

	response.services.Emplace();
	response.services->FromList(services);
	ApplyLayoutCoordinates(response);

	return response;
}


sdl::V1_0::agentino::CTopology CTopologyControllerComp::GetAgentSnapshot(
			const ::imtgql::CGqlRequest& gqlRequest,
			const QByteArray& agentId,
			bool& isFresh,
			QString& errorMessage) const
{
	isFresh = false;

	istd::TUniqueInterfacePtr<imtgql::CGqlRequest> gqlRequestPtr;
	gqlRequestPtr.MoveCastedPtr(gqlRequest.CloneMe());
	if (!gqlRequestPtr.IsValid()){
		return sdl::V1_0::agentino::CTopology();
	}

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequestPtr->SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::CTopology response = SendModelRequest<sdl::V1_0::agentino::CTopology>(*gqlRequestPtr.GetPtr(), errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, QString("Unable to load topology snapshot for agent '%1'. Error: %2").arg(qPrintable(agentId), errorMessage), "CTopologyControllerComp");
		return sdl::V1_0::agentino::CTopology();
	}

	isFresh = true;
	return response;
}


void CTopologyControllerComp::ApplyLayoutCoordinates(sdl::V1_0::agentino::CTopology& topology) const
{
	if (!topology.services.has_value()){
		return;
	}

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> services = *topology.services;
	for (istd::TNullableValue<sdl::V1_0::agentino::CService>& service : *services.GetPtr()){
		if (!service->id.has_value()){
			continue;
		}

		QPoint point = GetServiceCoordinate(*service->id);
		service->x = point.x();
		service->y = point.y();
	}

	topology.services = services;
}


void CTopologyControllerComp::MarkServicesAsStale(sdl::V1_0::agentino::CTopology& topology) const
{
	if (!topology.services.has_value()){
		return;
	}

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> services = *topology.services;
	for (istd::TNullableValue<sdl::V1_0::agentino::CService>& service : *services.GetPtr()){
		service->status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
		service->icon1 = "Icons/Alert";
		service->icon2 = "Icons/Warning";
		service->hasError = true;
	}

	topology.services = services;
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
