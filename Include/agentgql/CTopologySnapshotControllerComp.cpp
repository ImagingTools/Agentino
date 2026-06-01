// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CTopologySnapshotControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologySnapshotControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CTopologySnapshotControllerComp");
		return response;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	QList<sdl::V1_0::agentino::CService> services;
	imtbase::ICollectionInfo::Ids serviceElementIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!m_serviceCollectionCompPtr->GetObjectData(serviceElementId, serviceDataPtr)){
			continue;
		}

		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		sdl::V1_0::agentino::CService service;
		service.id = serviceElementId;
		service.agentId = agentId;
		service.mainText = serviceInfoPtr->GetServiceName();
		service.secondText = serviceInfoPtr->GetServiceDescription();
		service.thirdText = serviceInfoPtr->GetServiceTypeId();

		agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
		if (m_serviceControllerCompPtr.IsValid()){
			serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(serviceElementId);
		}

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

		QList<sdl::V1_0::agentino::CLink> linkList;
		imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
		if (connectionCollectionPtr != nullptr){
			imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (!connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
					continue;
				}

				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr == nullptr){
					continue;
				}

				QByteArray dependantConnectionId = connectionLinkParamPtr->GetDependantServiceConnectionId();
				QByteArray dependantServiceId = ResolveServiceIdByDependantConnectionId(dependantConnectionId);
				if (dependantServiceId.isEmpty()){
					continue;
				}

				sdl::V1_0::agentino::CLink linkData;
				linkData.id = dependantServiceId;
				linkList << linkData;
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
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CSaveTopologyResponse response;
	response.successful = false;

	return response;
}


QByteArray CTopologySnapshotControllerComp::ResolveServiceIdByDependantConnectionId(const QByteArray& dependantServiceConnectionId) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	imtbase::ICollectionInfo::Ids serviceElementIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!m_serviceCollectionCompPtr->GetObjectData(serviceElementId, serviceDataPtr)){
			continue;
		}

		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
		if (connectionCollectionPtr == nullptr){
			continue;
		}

		imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
			if (connectionElementId == dependantServiceConnectionId){
				return serviceElementId;
			}

			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = connectionParamPtr->GetIncomingConnections();
					for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
						if (incomingConnection.GetObjectUuid() == dependantServiceConnectionId){
							return serviceElementId;
						}
					}
				}
			}
		}
	}

	return QByteArray();
}


} // namespace agentgql
