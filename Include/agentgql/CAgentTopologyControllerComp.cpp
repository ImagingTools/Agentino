// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


// Qt includes
#include <QtCore/QPoint>

// ACF includes
#include <i2d/CPosition2d.h>

// ImtCore includes
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CAgentTopologyControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;

	QList<sdl::V1_0::agentino::CService> services;

	QByteArray agentId;
	if (m_clientIdCompPtr.IsValid()){
		agentId = m_clientIdCompPtr->GetText().toUtf8();
	}

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
		service.agentId = agentId;
		// Local topology is served by this agent - all listed services belong to the online host.
		service.agentOnline = true;

		QPoint point = GetServiceCoordinate(serviceElementId);

		service.x = point.x();
		service.y = point.y();

		QString serviceTypeName = serviceInfoPtr->GetServiceTypeId();
		QString serviceName = serviceInfoPtr->GetServiceName();
		QString description = serviceInfoPtr->GetServiceDescription();
		QString agentName = m_serviceCompositeInfoCompPtr->GetServiceAgentName(serviceElementId);

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
			// The state of dependant services of other agents cannot be resolved locally,
			// show a warning instead of an error
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
						QByteArray dependantConnectionId = connectionLinkParamPtr->GetDependantServiceConnectionId();
						QByteArray serviceId = m_serviceCompositeInfoCompPtr->GetServiceId(dependantConnectionId);
						if (serviceId.isEmpty()){
							// The dependant service belongs to another agent and cannot be resolved locally
							continue;
						}

						sdl::V1_0::agentino::CLink linkData;
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

	response.services.Emplace();
	response.services->FromList(services);

	return response;
}


sdl::V1_0::agentino::CSaveTopologyResponse CAgentTopologyControllerComp::OnSaveTopology(
			const sdl::V1_0::agentino::CSaveTopologyGqlRequest& saveTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSaveTopologyResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CAgentTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::SaveTopologyRequestArguments arguments = saveTopologyRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to save topology. Error: GraphQL version 1.0 is invalid");
		SendErrorMessage(0, errorMessage, "CAgentTopologyControllerComp");

		return response;
	}

	m_topologyCollectionCompPtr->ResetData();
	if (!arguments.input->services.has_value()){
		errorMessage = QString("Unable to save topology. Error: Input params is invalid");
		return response;
	}

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CService>> serviceList = *arguments.input->services;
	for (const istd::TNullableValue<sdl::V1_0::agentino::CService>& service : *serviceList.GetPtr()){
		if (!service->x.has_value() || !service->y.has_value() || !service->id.has_value()){
			errorMessage = QString("Unable to save topology. Error: Service is missing required fields (id, x, y)");
			SendErrorMessage(0, errorMessage, "CAgentTopologyControllerComp");
			return response;
		}

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


QPoint CAgentTopologyControllerComp::GetServiceCoordinate(const QByteArray& serviceId) const
{
	QPoint retVal;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CAgentTopologyControllerComp");
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
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CAgentTopologyControllerComp");
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


} // namespace agentgql
