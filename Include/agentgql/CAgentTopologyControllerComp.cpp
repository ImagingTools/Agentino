// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentTopology.h>


// Qt includes
#include <QtCore/QPoint>
#include <QtCore/QDateTime>
#include <QtNetwork/QHostInfo>

// ACF includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>


namespace agentgql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CAgentTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CLocalTopology CAgentTopologyControllerComp::OnGetLocalTopology(
			const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& /*getLocalTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CLocalTopology response;

	// Set agent identification
	QString clientId;
	if (m_clientIdCompPtr.IsValid()){
		clientId = m_clientIdCompPtr->GetText();
	}
	response.agentId = clientId.toUtf8();

	QString localHostName = QHostInfo::localHostName();
	QString domainName = QHostInfo::localDomainName();
	QString agentName = localHostName;
	if (!domainName.isEmpty()){
		agentName += "@" + domainName;
	}
	response.agentName = agentName;
	response.connectionStatus = sdl::V1_0::agentino::AgentConnectionStatus::ONLINE;
	response.lastSeen = QDateTime::currentDateTime().toString(Qt::ISODate);

	// Build service list
	QList<sdl::V1_0::agentino::CLocalServiceInfo> services;

	if (m_serviceCollectionCompPtr.IsValid()){
		imtbase::ICollectionInfo::Ids serviceIds = m_serviceCollectionCompPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			sdl::V1_0::agentino::CLocalServiceInfo serviceItem;
			serviceItem.id = serviceId;
			serviceItem.name = serviceInfoPtr->GetServiceName();
			serviceItem.description = serviceInfoPtr->GetServiceDescription();
			serviceItem.typeId = serviceInfoPtr->GetServiceTypeId();
			serviceItem.version = serviceInfoPtr->GetServiceVersion();
			serviceItem.isAutoStart = serviceInfoPtr->IsAutoStart();
			serviceItem.status = GetLocalServiceStatusEnum(serviceId);

			// Get coordinates from topology collection
			QPoint point = GetServiceCoordinate(serviceId);
			serviceItem.x = point.x();
			serviceItem.y = point.y();

			// Build dependency links
			QList<sdl::V1_0::agentino::CLocalServiceLink> linkList;
			imtbase::IObjectCollection* dependantConnections = serviceInfoPtr->GetDependantServiceConnections();
			if (dependantConnections != nullptr){
				imtbase::ICollectionInfo::Ids connectionIds = dependantConnections->GetElementIds();
				for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
					imtbase::IObjectCollection::DataPtr connectionDataPtr;
					if (dependantConnections->GetObjectData(connectionId, connectionDataPtr)){
						imtservice::CUrlConnectionLinkParam* linkParamPtr =
							dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
						if (linkParamPtr != nullptr){
							sdl::V1_0::agentino::CLocalServiceLink link;
							link.id = linkParamPtr->GetDependantServiceConnectionId();
							linkList << link;
						}
					}
				}
			}

			serviceItem.links.Emplace();
			serviceItem.links->FromList(linkList);

			services << serviceItem;
		}
	}

	response.services.Emplace();
	response.services->FromList(services);

	return response;
}


sdl::V1_0::agentino::CLocalServiceStatusResponse CAgentTopologyControllerComp::OnGetLocalServiceStatus(
			const sdl::V1_0::agentino::CGetLocalServiceStatusGqlRequest& getLocalServiceStatusRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CLocalServiceStatusResponse response;

	sdl::V1_0::agentino::GetLocalServiceStatusRequestArguments arguments = getLocalServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to get service status. Error: GraphQL version 1.0 is invalid");
		return response;
	}

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to get service status. Error: Service ID is empty");
		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	response.id = serviceId;
	response.status = GetLocalServiceStatusEnum(serviceId);

	return response;
}


sdl::V1_0::agentino::CSaveLocalTopologyResponse CAgentTopologyControllerComp::OnSaveLocalTopology(
			const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& saveLocalTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSaveLocalTopologyResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to save topology. Error: TopologyCollection not configured");
		return response;
	}

	sdl::V1_0::agentino::SaveLocalTopologyRequestArguments arguments = saveLocalTopologyRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to save topology. Error: GraphQL version 1.0 is invalid");
		return response;
	}

	if (!arguments.input->services.has_value()){
		errorMessage = QString("Unable to save topology. Error: Input services are invalid");
		return response;
	}

	m_topologyCollectionCompPtr->ResetData();

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CLocalServiceInfo>> serviceList = *arguments.input->services;
	for (const istd::TNullableValue<sdl::V1_0::agentino::CLocalServiceInfo>& service : *serviceList.GetPtr()){
		int x = service->x.has_value() ? static_cast<int>(*service->x) : 0;
		int y = service->y.has_value() ? static_cast<int>(*service->y) : 0;
		QByteArray id = *service->id;
		QPoint point(x, y);

		if (!SetServiceCoordinate(id, point)){
			response.successful = false;
			return response;
		}
	}

	response.successful = true;
	return response;
}


// private methods

QPoint CAgentTopologyControllerComp::GetServiceCoordinate(const QByteArray& serviceId) const
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


bool CAgentTopologyControllerComp::SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const
{
	if (!m_topologyCollectionCompPtr.IsValid()){
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


sdl::V1_0::agentino::LocalServiceStatus CAgentTopologyControllerComp::GetLocalServiceStatusEnum(const QByteArray& serviceId) const
{
	if (!m_serviceControllerCompPtr.IsValid()){
		return sdl::V1_0::agentino::LocalServiceStatus::UNDEFINED;
	}

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	switch (status){
		case agentinodata::IServiceStatusInfo::SS_RUNNING:
			return sdl::V1_0::agentino::LocalServiceStatus::RUNNING;
		case agentinodata::IServiceStatusInfo::SS_NOT_RUNNING:
			return sdl::V1_0::agentino::LocalServiceStatus::NOT_RUNNING;
		case agentinodata::IServiceStatusInfo::SS_STARTING:
			return sdl::V1_0::agentino::LocalServiceStatus::STARTING;
		case agentinodata::IServiceStatusInfo::SS_STOPPING:
			return sdl::V1_0::agentino::LocalServiceStatus::STOPPING;
		case agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
			return sdl::V1_0::agentino::LocalServiceStatus::RUNNING_IMPOSSIBLE;
		default:
			return sdl::V1_0::agentino::LocalServiceStatus::UNDEFINED;
	}
}


} // namespace agentgql
