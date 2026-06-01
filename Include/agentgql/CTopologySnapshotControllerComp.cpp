// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CTopologySnapshotControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


// Qt includes
#include <QtCore/QPoint>

// ImtCore includes
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologySnapshotControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;

	QList<sdl::V1_0::agentino::CService> services;

	imtbase::ICollectionInfo::Ids serviceElementIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceElementId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr == nullptr){
			continue;
		}

		sdl::V1_0::agentino::CService service;

		service.id = serviceElementId;

		QString serviceName = serviceInfoPtr->GetServiceName();
		QString description = serviceInfoPtr->GetServiceDescription();
		QString serviceTypeName = serviceInfoPtr->GetServiceTypeId();

		service.mainText = serviceName;
		service.secondText = description;
		service.thirdText = serviceTypeName;

		// Query service status from local controller
		if (m_serviceControllerCompPtr.IsValid()){
			agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(serviceElementId);
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
		}
		else{
			service.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
			service.icon1 = "Icons/Alert";
		}

		// Build dependency links
		QList<sdl::V1_0::agentino::CLink> linkList;

		imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
		if (connectionCollectionPtr != nullptr){
			imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
					imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
					if (connectionLinkParamPtr != nullptr){
						QByteArray dependantConnectionId = connectionLinkParamPtr->GetDependantServiceConnectionId();
						QByteArray serviceId = ResolveServiceIdByConnectionId(dependantConnectionId);

						if (!serviceId.isEmpty()){
							sdl::V1_0::agentino::CLink linkData;
							linkData.id = serviceId;
							linkList << linkData;
						}
					}
				}
			}
		}

		service.links.Emplace();
		service.links->FromList(linkList);

		services << service;
	}

	response.services.Emplace();
	response.services->FromList(services);

	return response;
}


sdl::V1_0::agentino::CSaveTopologyResponse CTopologySnapshotControllerComp::OnSaveTopology(
			const sdl::V1_0::agentino::CSaveTopologyGqlRequest& /*saveTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSaveTopologyResponse response;
	response.successful = false;
	errorMessage = "SaveTopology is not supported on the agent side. Layout is managed centrally.";
	return response;
}


QByteArray CTopologySnapshotControllerComp::ResolveServiceIdByConnectionId(const QByteArray& connectionId) const
{
	imtbase::ICollectionInfo::Ids serviceElementIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceElementId, serviceDataPtr)){
			agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr != nullptr){
				imtbase::IObjectCollection* inputConnections = serviceInfoPtr->GetInputConnections();
				if (inputConnections != nullptr){
					imtbase::ICollectionInfo::Ids inputConnectionIds = inputConnections->GetElementIds();
					if (inputConnectionIds.contains(connectionId)){
						return serviceElementId;
					}
				}
			}
		}
	}

	return QByteArray();
}


} // namespace agentgql
