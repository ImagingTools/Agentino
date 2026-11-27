// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTopologyControllerComp.h>


//Qt includes
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QtMath>

// ACF includes
#include <i2d/CPosition2d.h>
#include <istd/CChangeGroup.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IAgentStatusInfo.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/CTopologyGroup.h>
#include <agentinodata/CTopologyView.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinogql
{


// Type-ID of user-defined groups in the topology collection (see Repositories.acc).
static const QByteArray s_topologyGroupTypeId = QByteArrayLiteral("TopologyGroup");
// Type-ID of user-defined views in the topology collection (see Repositories.acc).
static const QByteArray s_topologyViewTypeId = QByteArrayLiteral("TopologyView");
// Element description marking a removed view. A removed view is kept with its groups, so that the removal
// can be undone; the mark is stored with the element and does not change the serialized view data.
static const QString s_removedTopologyViewDescription = QStringLiteral("#removed");


// Layout (x/y) is a view concern stored only in TopologyCollection on the server/GUI
// side — it is never pushed to agents or included in the agent↔server domain event stream
// (Architecture Audit §4.9 / Step 7).

// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologyControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& getTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;
	
	
	QList<sdl::V1_0::agentino::CService> services;
	QList<sdl::V1_0::agentino::CTopologyNodeInfo> availableServices;
	QList<sdl::V1_0::agentino::CService> availableServiceNodes;

	sdl::V1_0::agentino::GetTopologyRequestArguments arguments = getTopologyRequest.GetRequestedArguments();
	const QByteArray viewId = (arguments.input.has_value() && arguments.input->viewId.has_value()) ? *arguments.input->viewId : QByteArray();

	// The default view groups services by agents: agents are composite nodes and a service belongs
	// to its agent unless it was moved into a group. In a user-defined view services are grouped
	// freely and have their own positions.
	const bool isAgentsView = viewId.isEmpty();
	agentinodata::CTopologyView view;
	if (!isAgentsView && !GetTopologyView(viewId, view)){
		// The view was removed (e.g. by another client): nothing to show, the client returns to the default view.
		response.services.Emplace();
		response.services->FromList(services);

		return response;
	}

	const TopologyGroupMap groups = GetTopologyGroups(viewId);
		
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
				
				bool agentOnline = IsAgentOnline(elementId);

				if (isAgentsView){
					sdl::V1_0::agentino::CService agentNode;
					agentNode.id = elementId;
					agentNode.agentId = elementId;
					agentNode.nodeType = sdl::V1_0::agentino::TopologyNodeType::AGENT;
					agentNode.isComposite = true;
					agentNode.parentId = FindParentGroupId(elementId, groups);

					QPoint agentPoint = GetServiceCoordinate(elementId);
					agentNode.x = agentPoint.x();
					agentNode.y = agentPoint.y();

					agentNode.mainText = agentName;
					agentNode.secondText = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
					agentNode.thirdText = QStringLiteral("Agent");
					agentNode.agentOnline = agentOnline;
					agentNode.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
					agentNode.icon1 = agentOnline ? QString() : QStringLiteral("Icons/Alert");
					agentNode.icon2 = QString();
					agentNode.links.Emplace();
					agentNode.links->FromList(QList<sdl::V1_0::agentino::CLink>());

					services << agentNode;
				}
				
				imtbase::IObjectCollection* serviceCollectionPtr =
							m_serviceManagerCompPtr.IsValid()
										? m_serviceManagerCompPtr->GetServiceCollection(elementId)
										: nullptr;
				if (serviceCollectionPtr == nullptr){
					continue;
				}
				
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

					// A user-defined view shows only the services added to it. The other services are listed as
					// available ones; their nodes are sent as well, so that the client can add them to its draft.
					const bool isInView = isAgentsView || view.GetNodeIds().contains(serviceElementId);
					if (!isInView){
						sdl::V1_0::agentino::CTopologyNodeInfo availableService;
						availableService.id = serviceElementId;
						availableService.name = serviceInfoPtr->GetServiceName() + "@" + agentName;

						availableServices << availableService;
					}

					sdl::V1_0::agentino::CService service;

					service.id = serviceElementId;
					service.agentId = elementId;
					service.nodeType = sdl::V1_0::agentino::TopologyNodeType::SERVICE;
					service.isComposite = false;

					if (isInView){
						const QByteArray parentGroupId = FindParentGroupId(serviceElementId, groups);
						service.parentId = (parentGroupId.isEmpty() && isAgentsView) ? elementId : parentGroupId;
					}
					else{
						// Added to the view, the service is placed at the root level.
						service.parentId = QByteArray();
					}
					
					QPoint point;
					if (isAgentsView){
						point = GetServiceCoordinate(serviceElementId);
					}
					else{
						i2d::CVector2d position;
						if (view.GetNodePosition(serviceElementId, position)){
							point = QPoint(qRound(position.GetX()), qRound(position.GetY()));
						}
					}
					
					service.x = point.x();
					service.y = point.y();
					
					QString serviceTypeName = serviceInfoPtr->GetServiceTypeId();
					QString serviceName = serviceInfoPtr->GetServiceName();
					QString description = serviceInfoPtr->GetServiceDescription();

					service.mainText = serviceName + "@" + agentName;
					service.secondText = description;
					
					service.thirdText = serviceTypeName;
					
					service.agentOnline = agentOnline;
					
					agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceElementId);
					if (!agentOnline){
						// The agent is offline - the cached service data may be outdated,
						// the actual service status is unknown
						service.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
						service.icon1 = "Icons/Alert";
					}
					else if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
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
					if (!agentOnline){
						service.icon2 = "Icons/Warning";
					}
					else if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
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

					if (isInView){
						services << service;
					}
					else{
						availableServiceNodes << service;
					}
				}
			}
		}
	}

	for (TopologyGroupMap::const_iterator groupIter = groups.constBegin(); groupIter != groups.constEnd(); ++groupIter){
		const QByteArray& groupId = groupIter.key();

		sdl::V1_0::agentino::CService groupNode;
		groupNode.id = groupId;
		groupNode.agentId = QByteArray();
		groupNode.nodeType = sdl::V1_0::agentino::TopologyNodeType::GROUP;
		groupNode.isComposite = true;
		groupNode.parentId = FindParentGroupId(groupId, groups);

		QPoint groupPoint = GetServiceCoordinate(groupId);
		groupNode.x = groupPoint.x();
		groupNode.y = groupPoint.y();

		groupNode.mainText = groupIter->name;
		groupNode.secondText = QString();
		groupNode.thirdText = QStringLiteral("Group");
		groupNode.agentOnline = true;
		groupNode.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
		groupNode.icon1 = QString();
		groupNode.icon2 = QString();
		groupNode.links.Emplace();
		groupNode.links->FromList(QList<sdl::V1_0::agentino::CLink>());

		services << groupNode;
	}

	response.services.Emplace();
	response.services->FromList(services);

	response.availableServices.Emplace();
	response.availableServices->FromList(availableServices);

	response.availableServiceNodes.Emplace();
	response.availableServiceNodes->FromList(availableServiceNodes);

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

	// Never ResetData() here: TopologySubscriber publishes OnTopologyChanged on every
	// collection change, the client reloads GetTopology mid-write and snaps nodes back
	// to (0,0) / previous coords while the collection is empty.
	if (!arguments.input->services.has_value()){
		errorMessage = QString("Unable to save topology. Error: Input params is invalid");
		return response;
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	// In a user-defined view the groups keep their own positions, the positions of the services
	// are stored in the view (the same service has different positions in different views).
	const QByteArray viewId = arguments.input->viewId.has_value() ? *arguments.input->viewId : QByteArray();
	agentinodata::CTopologyView view;
	if (!viewId.isEmpty() && !GetTopologyView(viewId, view)){
		errorMessage = QString("Unable to save topology. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
		return response;
	}

	// The client edits the view locally and saves its whole state at once: the groups (and the services
	// of a user-defined view) are replaced first, then the positions of all nodes are stored.
	if (arguments.input->groups.has_value()){
		QList<TopologyGroupState> groupStates;

		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CTopologyGroupInput>> groupList = *arguments.input->groups;
		for (const istd::TNullableValue<sdl::V1_0::agentino::CTopologyGroupInput>& groupInput : *groupList.GetPtr()){
			if (!groupInput.has_value()
						|| !groupInput->id.has_value() || groupInput->id->isEmpty()
						|| !groupInput->name.has_value() || groupInput->name->trimmed().isEmpty()){
				errorMessage = QString("Unable to save topology. Error: Group is missing required fields (id, name)");
				SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
				return response;
			}

			TopologyGroupState groupState;
			groupState.id = *groupInput->id;
			groupState.name = groupInput->name->trimmed();
			groupState.position.SetX(groupInput->x.has_value() ? qRound(*groupInput->x) : 0);
			groupState.position.SetY(groupInput->y.has_value() ? qRound(*groupInput->y) : 0);
			if (groupInput->memberIds.has_value()){
				groupState.memberIds = (*groupInput->memberIds).ToList();
			}

			groupStates << groupState;
		}

		QByteArrayList viewNodeIds;
		const bool hasViewNodeIds = !viewId.isEmpty() && arguments.input->nodeIds.has_value();
		if (hasViewNodeIds){
			viewNodeIds = (*arguments.input->nodeIds).ToList();
		}

		if (!SaveTopologyViewState(viewId, groupStates, hasViewNodeIds ? &viewNodeIds : nullptr, errorMessage)){
			errorMessage = QString("Unable to save topology. Error: %1").arg(errorMessage);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
			return response;
		}

		// The view was changed (groups, services).
		if (!viewId.isEmpty() && !GetTopologyView(viewId, view)){
			errorMessage = QString("Unable to save topology. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
			return response;
		}
	}

	TopologyGroupMap viewGroups;
	if (!viewId.isEmpty()){
		viewGroups = GetTopologyGroups(viewId);
	}

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CServiceCoordinateInput>> serviceList = *arguments.input->services;
	for (const istd::TNullableValue<sdl::V1_0::agentino::CServiceCoordinateInput>& service : *serviceList.GetPtr()){
		if (!service.has_value()
					|| !service->id.has_value() || service->id->isEmpty()
					|| !service->x.has_value()
					|| !service->y.has_value()){
			errorMessage = QString("Unable to save topology. Error: Service is missing required fields (id, x, y)");
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
			return response;
		}

		const QByteArray id = *service->id;
		// SDL x/y are Double; store as rounded ints for QPoint / CVector2d layout grid.
		const QPoint point(qRound(*service->x), qRound(*service->y));
		if (!viewId.isEmpty() && !viewGroups.contains(id)){
			view.SetNodePosition(id, i2d::CVector2d(point.x(), point.y()));
			continue;
		}

		if (!SetServiceCoordinate(id, point)){
			errorMessage = QString("Unable to save topology. Error: Failed to store coordinates for '%1'")
						.arg(QString::fromUtf8(id));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
			return response;
		}
	}

	if (!viewId.isEmpty() && !m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
		errorMessage = QString("Unable to save topology. Error: Failed to store the layout of the view '%1'").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
		return response;
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CCreateTopologyGroupResponse CTopologyControllerComp::OnCreateTopologyGroup(
			const sdl::V1_0::agentino::CCreateTopologyGroupGqlRequest& createTopologyGroupRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CCreateTopologyGroupResponse response;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::CreateTopologyGroupRequestArguments arguments = createTopologyGroupRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->name.has_value() || arguments.input->name->trimmed().isEmpty()){
		errorMessage = QString("Unable to create topology group. Error: Group name is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QString name = arguments.input->name->trimmed();
	const QByteArray parentId = arguments.input->parentId.has_value() ? *arguments.input->parentId : QByteArray();
	const QByteArray viewId = arguments.input->viewId.has_value() ? *arguments.input->viewId : QByteArray();
	// A given ID recreates a removed group (undo of the removal, redo of the creation).
	const QByteArray proposedGroupId = arguments.input->id.has_value() ? *arguments.input->id : QByteArray();
	if (!proposedGroupId.isEmpty() && m_topologyCollectionCompPtr->GetElementIds().contains(proposedGroupId)){
		errorMessage = QString("Unable to create topology group. Error: Element '%1' already exists").arg(QString::fromUtf8(proposedGroupId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	agentinodata::CTopologyView view;
	if (!viewId.isEmpty() && !GetTopologyView(viewId, view)){
		errorMessage = QString("Unable to create topology group. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const TopologyGroupMap groups = GetTopologyGroups(viewId);
	if (!parentId.isEmpty() && !groups.contains(parentId)){
		errorMessage = QString("Unable to create topology group. Error: Parent group '%1' does not exist").arg(QString::fromUtf8(parentId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	i2d::CVector2d position;
	position.SetX(arguments.input->x.has_value() ? qRound(*arguments.input->x) : 0);
	position.SetY(arguments.input->y.has_value() ? qRound(*arguments.input->y) : 0);

	agentinodata::CTopologyGroup group;
	group.SetPosition(position);

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	const QByteArray groupId = m_topologyCollectionCompPtr->InsertNewObject(s_topologyGroupTypeId, name, "", &group, proposedGroupId);
	if (groupId.isEmpty() || (!proposedGroupId.isEmpty() && (groupId != proposedGroupId))){
		if (!groupId.isEmpty()){
			m_topologyCollectionCompPtr->RemoveElements(imtbase::ICollectionInfo::Ids() << groupId);
		}

		errorMessage = QString("Unable to create topology group. Error: Failed to insert group '%1'").arg(name);
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	if (!viewId.isEmpty()){
		QByteArrayList viewGroupIds = view.GetGroupIds();
		viewGroupIds.append(groupId);
		view.SetGroupIds(viewGroupIds);
		if (!m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
			m_topologyCollectionCompPtr->RemoveElements(imtbase::ICollectionInfo::Ids() << groupId);

			errorMessage = QString("Unable to create topology group. Error: Failed to add group to the view '%1'").arg(QString::fromUtf8(viewId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	if (!parentId.isEmpty()){
		QByteArrayList parentMemberIds = groups.value(parentId).memberIds;
		parentMemberIds.append(groupId);
		if (!SetTopologyGroupMembers(parentId, parentMemberIds)){
			errorMessage = QString("Unable to create topology group. Error: Failed to add group to '%1'").arg(QString::fromUtf8(parentId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	response.id = groupId;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnMoveTopologyNodes(
			const sdl::V1_0::agentino::CMoveTopologyNodesGqlRequest& moveTopologyNodesRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::MoveTopologyNodesRequestArguments arguments = moveTopologyNodesRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->nodeIds.has_value()){
		errorMessage = QString("Unable to move topology nodes. Error: Node list is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArrayList nodeIds = (*arguments.input->nodeIds).ToList();
	const QByteArray parentId = arguments.input->parentId.has_value() ? *arguments.input->parentId : QByteArray();
	const QByteArray viewId = arguments.input->viewId.has_value() ? *arguments.input->viewId : QByteArray();

	// Group membership is defined per view: the node is moved within the groups of the given view only.
	const TopologyGroupMap groups = GetTopologyGroups(viewId);
	if (!parentId.isEmpty() && !groups.contains(parentId)){
		errorMessage = QString("Unable to move topology nodes. Error: Target group '%1' does not exist").arg(QString::fromUtf8(parentId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	for (const QByteArray& nodeId : nodeIds){
		if (nodeId.isEmpty()){
			errorMessage = QString("Unable to move topology nodes. Error: Node ID is empty");
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}

		if (!parentId.isEmpty() && groups.contains(nodeId) && IsGroupInside(parentId, nodeId, groups)){
			errorMessage = QString("Unable to move topology nodes. Error: Group '%1' cannot be moved into itself or into its subgroup")
						.arg(groups.value(nodeId).name);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	// A node has only one parent: remove it from all groups, then add it to the target group.
	QMap<QByteArray, QByteArrayList> changedGroupMembers;
	for (TopologyGroupMap::const_iterator groupIter = groups.constBegin(); groupIter != groups.constEnd(); ++groupIter){
		QByteArrayList memberIds = groupIter->memberIds;

		bool isChanged = false;
		for (const QByteArray& nodeId : nodeIds){
			if (memberIds.removeAll(nodeId) > 0){
				isChanged = true;
			}
		}

		if (groupIter.key() == parentId){
			for (const QByteArray& nodeId : nodeIds){
				if (!memberIds.contains(nodeId)){
					memberIds.append(nodeId);
				}
			}

			isChanged = true;
		}

		if (isChanged){
			changedGroupMembers.insert(groupIter.key(), memberIds);
		}
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	for (QMap<QByteArray, QByteArrayList>::const_iterator iter = changedGroupMembers.constBegin(); iter != changedGroupMembers.constEnd(); ++iter){
		if (!SetTopologyGroupMembers(iter.key(), iter.value())){
			errorMessage = QString("Unable to move topology nodes. Error: Failed to update group '%1'").arg(groups.value(iter.key()).name);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnRemoveTopologyGroup(
			const sdl::V1_0::agentino::CRemoveTopologyGroupGqlRequest& removeTopologyGroupRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::RemoveTopologyGroupRequestArguments arguments = removeTopologyGroupRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value() || arguments.input->id->isEmpty()){
		errorMessage = QString("Unable to remove topology group. Error: Group ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray groupId = *arguments.input->id;

	const QByteArray viewId = FindGroupViewId(groupId);
	const TopologyGroupMap groups = GetTopologyGroups(viewId);
	if (!groups.contains(groupId)){
		errorMessage = QString("Unable to remove topology group. Error: Group '%1' does not exist").arg(QString::fromUtf8(groupId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	// The members of the removed group are moved one level up. Members of a root group
	// fall back to their default place (in the default view services go to their agents,
	// otherwise all nodes go to the root level).
	const QByteArray parentGroupId = FindParentGroupId(groupId, groups);
	if (!parentGroupId.isEmpty()){
		QByteArrayList parentMemberIds = groups.value(parentGroupId).memberIds;
		parentMemberIds.removeAll(groupId);
		for (const QByteArray& memberId : groups.value(groupId).memberIds){
			if (!parentMemberIds.contains(memberId)){
				parentMemberIds.append(memberId);
			}
		}

		if (!SetTopologyGroupMembers(parentGroupId, parentMemberIds)){
			errorMessage = QString("Unable to remove topology group. Error: Failed to update group '%1'").arg(groups.value(parentGroupId).name);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	if (!m_topologyCollectionCompPtr->RemoveElements(imtbase::ICollectionInfo::Ids() << groupId)){
		errorMessage = QString("Unable to remove topology group. Error: Failed to remove group '%1'").arg(groups.value(groupId).name);
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	agentinodata::CTopologyView view;
	if (!viewId.isEmpty() && GetTopologyView(viewId, view)){
		QByteArrayList viewGroupIds = view.GetGroupIds();
		viewGroupIds.removeAll(groupId);
		view.SetGroupIds(viewGroupIds);
		if (!m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
			errorMessage = QString("Unable to remove topology group. Error: Failed to update the view '%1'").arg(QString::fromUtf8(viewId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnRenameTopologyGroup(
			const sdl::V1_0::agentino::CRenameTopologyGroupGqlRequest& renameTopologyGroupRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::RenameTopologyGroupRequestArguments arguments = renameTopologyGroupRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value() || arguments.input->id->isEmpty()){
		errorMessage = QString("Unable to rename topology group. Error: Group ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	if (!arguments.input->name.has_value() || arguments.input->name->trimmed().isEmpty()){
		errorMessage = QString("Unable to rename topology group. Error: Group name is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray groupId = *arguments.input->id;
	if (m_topologyCollectionCompPtr->GetObjectTypeId(groupId) != s_topologyGroupTypeId){
		errorMessage = QString("Unable to rename topology group. Error: Group '%1' does not exist").arg(QString::fromUtf8(groupId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	// The group name is the element name in the topology collection.
	if (!m_topologyCollectionCompPtr->SetElementName(groupId, arguments.input->name->trimmed())){
		errorMessage = QString("Unable to rename topology group. Error: Failed to rename group '%1'").arg(QString::fromUtf8(groupId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnRemoveFromTopologyView(
			const sdl::V1_0::agentino::CRemoveFromTopologyViewGqlRequest& removeFromTopologyViewRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::RemoveFromTopologyViewRequestArguments arguments = removeFromTopologyViewRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->viewId.has_value() || arguments.input->viewId->isEmpty()){
		// The default view ("Agents") shows all services; a service can be removed from it only physically.
		errorMessage = QString("Unable to remove nodes from topology view. Error: View ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	if (!arguments.input->nodeIds.has_value() || arguments.input->nodeIds->isEmpty()){
		errorMessage = QString("Unable to remove nodes from topology view. Error: Node list is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray viewId = *arguments.input->viewId;
	const QByteArrayList nodeIds = (*arguments.input->nodeIds).ToList();

	agentinodata::CTopologyView view;
	if (!GetTopologyView(viewId, view)){
		errorMessage = QString("Unable to remove nodes from topology view. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const TopologyGroupMap groups = GetTopologyGroups(viewId);
	for (const QByteArray& nodeId : nodeIds){
		if (nodeId.isEmpty() || groups.contains(nodeId)){
			// Groups are removed with RemoveTopologyGroup
			errorMessage = QString("Unable to remove nodes from topology view. Error: '%1' is not a service").arg(QString::fromUtf8(nodeId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	for (TopologyGroupMap::const_iterator groupIter = groups.constBegin(); groupIter != groups.constEnd(); ++groupIter){
		QByteArrayList memberIds = groupIter->memberIds;

		bool isChanged = false;
		for (const QByteArray& nodeId : nodeIds){
			if (memberIds.removeAll(nodeId) > 0){
				isChanged = true;
			}
		}

		if (isChanged && !SetTopologyGroupMembers(groupIter.key(), memberIds)){
			errorMessage = QString("Unable to remove nodes from topology view. Error: Failed to update group '%1'").arg(groupIter->name);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	QByteArrayList viewNodeIds = view.GetNodeIds();
	for (const QByteArray& nodeId : nodeIds){
		viewNodeIds.removeAll(nodeId);
	}

	view.SetNodeIds(viewNodeIds);
	if (!m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
		errorMessage = QString("Unable to remove nodes from topology view. Error: Failed to update the view '%1'").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnAddToTopologyView(
			const sdl::V1_0::agentino::CAddToTopologyViewGqlRequest& addToTopologyViewRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::AddToTopologyViewRequestArguments arguments = addToTopologyViewRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->viewId.has_value() || arguments.input->viewId->isEmpty()){
		errorMessage = QString("Unable to add nodes to topology view. Error: View ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	if (!arguments.input->nodeIds.has_value() || arguments.input->nodeIds->isEmpty()){
		errorMessage = QString("Unable to add nodes to topology view. Error: Node list is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray viewId = *arguments.input->viewId;
	const QByteArrayList nodeIds = (*arguments.input->nodeIds).ToList();
	const QByteArray parentId = arguments.input->parentId.has_value() ? *arguments.input->parentId : QByteArray();

	agentinodata::CTopologyView view;
	if (!GetTopologyView(viewId, view)){
		errorMessage = QString("Unable to add nodes to topology view. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const TopologyGroupMap groups = GetTopologyGroups(viewId);
	if (!parentId.isEmpty() && !groups.contains(parentId)){
		errorMessage = QString("Unable to add nodes to topology view. Error: Target group '%1' does not exist").arg(QString::fromUtf8(parentId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	QByteArrayList viewNodeIds = view.GetNodeIds();
	QByteArrayList addedNodeIds;
	for (const QByteArray& nodeId : nodeIds){
		if (!nodeId.isEmpty() && !viewNodeIds.contains(nodeId) && !groups.contains(nodeId)){
			viewNodeIds.append(nodeId);
			addedNodeIds.append(nodeId);
		}
	}

	if (addedNodeIds.isEmpty()){
		// Already shown in the view (e.g. added by another client)
		response.successful = true;

		return response;
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	if (!parentId.isEmpty()){
		// Services not shown in the view are not members of its groups, so they are just appended to the target group.
		QByteArrayList memberIds = groups.value(parentId).memberIds;
		for (const QByteArray& nodeId : addedNodeIds){
			if (!memberIds.contains(nodeId)){
				memberIds.append(nodeId);
			}
		}

		if (!SetTopologyGroupMembers(parentId, memberIds)){
			errorMessage = QString("Unable to add nodes to topology view. Error: Failed to update group '%1'").arg(groups.value(parentId).name);
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	view.SetNodeIds(viewNodeIds);
	if (!m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
		errorMessage = QString("Unable to add nodes to topology view. Error: Failed to update the view '%1'").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyViewList CTopologyControllerComp::OnGetTopologyViews(
			const sdl::V1_0::agentino::CGetTopologyViewsGqlRequest& /*getTopologyViewsRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopologyViewList response;

	QList<sdl::V1_0::agentino::CTopologyViewInfo> views;

	if (m_topologyCollectionCompPtr.IsValid()){
		// Names of the content of the views: they are found by the names of their services and groups as well.
		QMap<QByteArray, QString> serviceNames;
		QStringList agentNames;
		CollectNodeNames(serviceNames, agentNames);

		const QByteArrayList viewIds = GetTopologyViewIds();
		for (const QByteArray& viewId : viewIds){
			sdl::V1_0::agentino::CTopologyViewInfo viewInfo;
			viewInfo.id = viewId;
			viewInfo.name = m_topologyCollectionCompPtr->GetElementInfo(viewId, imtbase::ICollectionInfo::EIT_NAME).toString();

			QStringList nodeNames;
			agentinodata::CTopologyView view;
			if (GetTopologyView(viewId, view)){
				for (const QByteArray& nodeId : view.GetNodeIds()){
					if (serviceNames.contains(nodeId)){
						nodeNames << serviceNames.value(nodeId);
					}
				}
			}

			const TopologyGroupMap groups = GetTopologyGroups(viewId);
			for (TopologyGroupMap::const_iterator groupIter = groups.constBegin(); groupIter != groups.constEnd(); ++groupIter){
				nodeNames << groupIter->name;
			}

			viewInfo.nodeNames.Emplace();
			viewInfo.nodeNames->FromList(nodeNames);

			views << viewInfo;
		}

		QStringList agentsViewNodeNames = agentNames;
		agentsViewNodeNames << serviceNames.values();
		const TopologyGroupMap defaultGroups = GetTopologyGroups(QByteArray());
		for (TopologyGroupMap::const_iterator groupIter = defaultGroups.constBegin(); groupIter != defaultGroups.constEnd(); ++groupIter){
			agentsViewNodeNames << groupIter->name;
		}

		response.agentsViewNodeNames.Emplace();
		response.agentsViewNodeNames->FromList(agentsViewNodeNames);
	}

	response.views.Emplace();
	response.views->FromList(views);

	return response;
}


sdl::V1_0::agentino::CCreateTopologyViewResponse CTopologyControllerComp::OnCreateTopologyView(
			const sdl::V1_0::agentino::CCreateTopologyViewGqlRequest& createTopologyViewRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CCreateTopologyViewResponse response;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::CreateTopologyViewRequestArguments arguments = createTopologyViewRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->name.has_value() || arguments.input->name->trimmed().isEmpty()){
		errorMessage = QString("Unable to create topology view. Error: View name is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QString name = arguments.input->name->trimmed();

	agentinodata::CTopologyView view;
	const QByteArray viewId = m_topologyCollectionCompPtr->InsertNewObject(s_topologyViewTypeId, name, "", &view);
	if (viewId.isEmpty()){
		errorMessage = QString("Unable to create topology view. Error: Failed to insert view '%1'").arg(name);
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	response.id = viewId;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnRemoveTopologyView(
			const sdl::V1_0::agentino::CRemoveTopologyViewGqlRequest& removeTopologyViewRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::RemoveTopologyViewRequestArguments arguments = removeTopologyViewRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value() || arguments.input->id->isEmpty()){
		errorMessage = QString("Unable to remove topology view. Error: View ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray viewId = *arguments.input->id;

	agentinodata::CTopologyView view;
	if (!GetTopologyView(viewId, view)){
		errorMessage = QString("Unable to remove topology view. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	// The view is marked as removed and kept with its groups, so that the removal can be undone
	// (RestoreTopologyView); it is not listed and not shown anymore. The services are not affected.
	if (!m_topologyCollectionCompPtr->SetElementDescription(viewId, s_removedTopologyViewDescription)){
		errorMessage = QString("Unable to remove topology view. Error: Failed to remove view '%1'").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	response.successful = true;

	return response;
}


sdl::V1_0::agentino::CTopologyGroupResponse CTopologyControllerComp::OnRestoreTopologyView(
			const sdl::V1_0::agentino::CRestoreTopologyViewGqlRequest& restoreTopologyViewRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CTopologyGroupResponse response;
	response.successful = false;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return response;
	}

	sdl::V1_0::agentino::RestoreTopologyViewRequestArguments arguments = restoreTopologyViewRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value() || arguments.input->id->isEmpty()){
		errorMessage = QString("Unable to restore topology view. Error: View ID is empty");
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	const QByteArray viewId = *arguments.input->id;

	agentinodata::CTopologyView view;
	if (!GetTopologyView(viewId, view, true)){
		errorMessage = QString("Unable to restore topology view. Error: View '%1' does not exist").arg(QString::fromUtf8(viewId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	if (IsTopologyViewRemoved(viewId)){
		if (!m_topologyCollectionCompPtr->SetElementDescription(viewId, QString())){
			errorMessage = QString("Unable to restore topology view. Error: Failed to restore view '%1'").arg(QString::fromUtf8(viewId));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

			return response;
		}
	}

	response.successful = true;

	return response;
}


bool CTopologyControllerComp::SaveTopologyViewState(
			const QByteArray& viewId,
			const QList<TopologyGroupState>& groupStates,
			const QByteArrayList* viewNodeIdsPtr,
			QString& errorMessage) const
{
	agentinodata::CTopologyView view;
	if (!viewId.isEmpty() && !GetTopologyView(viewId, view)){
		errorMessage = QString("View '%1' does not exist").arg(QString::fromUtf8(viewId));
		return false;
	}

	const TopologyGroupMap currentGroups = GetTopologyGroups(viewId);
	const imtbase::ICollectionInfo::Ids elementIds = m_topologyCollectionCompPtr->GetElementIds();

	// Members of the groups of a user-defined view are its services (and its groups).
	QSet<QByteArray> viewServiceIds;
	if (!viewId.isEmpty()){
		const QByteArrayList serviceIds = (viewNodeIdsPtr != nullptr) ? *viewNodeIdsPtr : view.GetNodeIds();
		for (const QByteArray& serviceId : serviceIds){
			viewServiceIds.insert(serviceId);
		}
	}

	QSet<QByteArray> groupIds;
	for (const TopologyGroupState& groupState : groupStates){
		if (groupIds.contains(groupState.id)){
			errorMessage = QString("Group '%1' is listed twice").arg(groupState.name);
			return false;
		}

		// A new group must not take the ID of another element (e.g. of a group of another view).
		if (!currentGroups.contains(groupState.id) && elementIds.contains(groupState.id)){
			errorMessage = QString("Element '%1' already exists").arg(QString::fromUtf8(groupState.id));
			return false;
		}

		groupIds.insert(groupState.id);
	}

	// A node belongs to one group only.
	QMap<QByteArray, QByteArray> parentGroupIds;
	QList<QByteArrayList> groupMemberIds;
	for (const TopologyGroupState& groupState : groupStates){
		QByteArrayList memberIds;
		for (const QByteArray& memberId : groupState.memberIds){
			if (memberId.isEmpty() || memberId == groupState.id){
				errorMessage = QString("Group '%1' has an invalid member").arg(groupState.name);
				return false;
			}

			if (parentGroupIds.contains(memberId)){
				errorMessage = QString("Node '%1' belongs to several groups").arg(QString::fromUtf8(memberId));
				return false;
			}

			const bool isGroup = groupIds.contains(memberId);
			if (!isGroup && m_topologyCollectionCompPtr->GetObjectTypeId(memberId) == s_topologyGroupTypeId){
				errorMessage = QString("Group '%1' is not a group of the view").arg(QString::fromUtf8(memberId));
				return false;
			}

			// A service that is not shown in the user-defined view cannot be a member of its groups.
			if (!viewId.isEmpty() && !isGroup && !viewServiceIds.contains(memberId)){
				continue;
			}

			parentGroupIds.insert(memberId, groupState.id);
			memberIds.append(memberId);
		}

		groupMemberIds << memberIds;
	}

	// Groups cannot be nested into themselves.
	for (const QByteArray& groupId : groupIds){
		QSet<QByteArray> visitedGroupIds;
		QByteArray currentGroupId = groupId;
		while (parentGroupIds.contains(currentGroupId)){
			currentGroupId = parentGroupIds.value(currentGroupId);
			if (currentGroupId == groupId || visitedGroupIds.contains(currentGroupId)){
				errorMessage = QString("Group '%1' cannot be nested into itself").arg(QString::fromUtf8(groupId));
				return false;
			}

			visitedGroupIds.insert(currentGroupId);
		}
	}

	// The groups that are not listed anymore are removed; their members are listed in other groups or
	// are at their default place now.
	imtbase::ICollectionInfo::Ids removedGroupIds;
	for (TopologyGroupMap::const_iterator groupIter = currentGroups.constBegin(); groupIter != currentGroups.constEnd(); ++groupIter){
		if (!groupIds.contains(groupIter.key())){
			removedGroupIds << groupIter.key();
		}
	}

	if (!removedGroupIds.isEmpty() && !m_topologyCollectionCompPtr->RemoveElements(removedGroupIds)){
		errorMessage = QString("Failed to remove groups");
		return false;
	}

	QByteArrayList viewGroupIds;
	for (int groupIndex = 0; groupIndex < groupStates.count(); ++groupIndex){
		const TopologyGroupState& groupState = groupStates[groupIndex];

		agentinodata::CTopologyGroup group;
		group.SetPosition(groupState.position);
		group.SetMemberIds(groupMemberIds[groupIndex]);

		if (currentGroups.contains(groupState.id)){
			if (!m_topologyCollectionCompPtr->SetObjectData(groupState.id, group)){
				errorMessage = QString("Failed to update group '%1'").arg(groupState.name);
				return false;
			}

			// The group name is the element name in the topology collection.
			if ((currentGroups.value(groupState.id).name != groupState.name)
						&& !m_topologyCollectionCompPtr->SetElementName(groupState.id, groupState.name)){
				errorMessage = QString("Failed to rename group '%1'").arg(groupState.name);
				return false;
			}
		}
		else{
			const QByteArray insertedGroupId = m_topologyCollectionCompPtr->InsertNewObject(s_topologyGroupTypeId, groupState.name, "", &group, groupState.id);
			if (insertedGroupId != groupState.id){
				if (!insertedGroupId.isEmpty()){
					m_topologyCollectionCompPtr->RemoveElements(imtbase::ICollectionInfo::Ids() << insertedGroupId);
				}

				errorMessage = QString("Failed to insert group '%1'").arg(groupState.name);
				return false;
			}
		}

		viewGroupIds << groupState.id;
	}

	if (!viewId.isEmpty()){
		view.SetGroupIds(viewGroupIds);
		if (viewNodeIdsPtr != nullptr){
			view.SetNodeIds(*viewNodeIdsPtr);
		}

		if (!m_topologyCollectionCompPtr->SetObjectData(viewId, view)){
			errorMessage = QString("Failed to update the view '%1'").arg(QString::fromUtf8(viewId));
			return false;
		}
	}

	return true;
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

	if (serviceId.isEmpty()){
		return false;
	}

	i2d::CPosition2d position2d;
	i2d::CVector2d position;

	position.SetX(point.x());
	position.SetY(point.y());
	position2d.SetPosition(position);

	// Upsert: InsertNewObject fails (or renames) when the id already exists.
	if (m_topologyCollectionCompPtr->GetElementIds().contains(serviceId)){
		return m_topologyCollectionCompPtr->SetObjectData(serviceId, position2d);
	}

	const QByteArray retVal = m_topologyCollectionCompPtr->InsertNewObject(
				"Topology", "", "", &position2d, serviceId);
	return !retVal.isEmpty();
}


bool CTopologyControllerComp::IsAgentOnline(const QByteArray& agentId) const
{
	if (!m_agentStatusCollectionCompPtr.IsValid()){
		// No agent status information available - assume the agent is online
		return true;
	}
	
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_agentStatusCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::IAgentStatusInfo* agentStatusInfoPtr = dynamic_cast<agentinodata::IAgentStatusInfo*>(dataPtr.GetPtr());
		if (agentStatusInfoPtr != nullptr){
			return agentStatusInfoPtr->GetAgentStatus() == agentinodata::IAgentStatusInfo::AS_CONNECTED;
		}
	}
	
	// No explicit status found - assume the agent is online
	return true;
}


void CTopologyControllerComp::CollectNodeNames(QMap<QByteArray, QString>& serviceNames, QStringList& agentNames) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		const QString agentName = m_agentCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
		agentNames << agentName;

		imtbase::IObjectCollection* serviceCollectionPtr =
					m_serviceManagerCompPtr.IsValid()
								? m_serviceManagerCompPtr->GetServiceCollection(agentId)
								: nullptr;
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
				if (serviceInfoPtr != nullptr){
					// The same text as on the scheme: "<service name>@<agent name>".
					serviceNames.insert(serviceId, serviceInfoPtr->GetServiceName() + "@" + agentName);
				}
			}
		}
	}
}


QByteArrayList CTopologyControllerComp::GetTopologyViewIds(bool includeRemoved) const
{
	QByteArrayList retVal;

	if (!m_topologyCollectionCompPtr.IsValid()){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids elementIds = m_topologyCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
		if (m_topologyCollectionCompPtr->GetObjectTypeId(elementId) == s_topologyViewTypeId){
			if (includeRemoved || !IsTopologyViewRemoved(elementId)){
				retVal.append(elementId);
			}
		}
	}

	return retVal;
}


bool CTopologyControllerComp::GetTopologyView(const QByteArray& viewId, agentinodata::CTopologyView& view, bool includeRemoved) const
{
	if (!m_topologyCollectionCompPtr.IsValid() || viewId.isEmpty()){
		return false;
	}

	if (m_topologyCollectionCompPtr->GetObjectTypeId(viewId) != s_topologyViewTypeId){
		return false;
	}

	if (!includeRemoved && IsTopologyViewRemoved(viewId)){
		return false;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_topologyCollectionCompPtr->GetObjectData(viewId, dataPtr)){
		return false;
	}

	const agentinodata::CTopologyView* viewPtr = dynamic_cast<const agentinodata::CTopologyView*>(dataPtr.GetPtr());

	return (viewPtr != nullptr) && view.CopyFrom(*viewPtr);
}


bool CTopologyControllerComp::IsTopologyViewRemoved(const QByteArray& viewId) const
{
	return m_topologyCollectionCompPtr->GetElementInfo(viewId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString() == s_removedTopologyViewDescription;
}


QByteArray CTopologyControllerComp::FindGroupViewId(const QByteArray& groupId) const
{
	// Removed views keep their groups (the removal can be undone).
	const QByteArrayList viewIds = GetTopologyViewIds(true);
	for (const QByteArray& viewId : viewIds){
		agentinodata::CTopologyView view;
		if (GetTopologyView(viewId, view, true) && view.GetGroupIds().contains(groupId)){
			return viewId;
		}
	}

	return QByteArray();
}


CTopologyControllerComp::TopologyGroupMap CTopologyControllerComp::GetTopologyGroups(const QByteArray& viewId) const
{
	TopologyGroupMap retVal;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return retVal;
	}

	// Groups of a user-defined view are listed in the view; the default view owns all other groups.
	QSet<QByteArray> viewGroupIds;
	if (viewId.isEmpty()){
		// The groups of removed views are not returned to the default view.
		const QByteArrayList viewIds = GetTopologyViewIds(true);
		for (const QByteArray& otherViewId : viewIds){
			agentinodata::CTopologyView otherView;
			if (GetTopologyView(otherViewId, otherView, true)){
				for (const QByteArray& groupId : otherView.GetGroupIds()){
					viewGroupIds.insert(groupId);
				}
			}
		}
	}
	else{
		agentinodata::CTopologyView view;
		if (!GetTopologyView(viewId, view)){
			return retVal;
		}

		for (const QByteArray& groupId : view.GetGroupIds()){
			viewGroupIds.insert(groupId);
		}
	}

	const imtbase::ICollectionInfo::Ids elementIds = m_topologyCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
		if (m_topologyCollectionCompPtr->GetObjectTypeId(elementId) != s_topologyGroupTypeId){
			continue;
		}

		// Default view: skip groups of user-defined views; user-defined view: skip groups not listed in it.
		const bool isListedGroup = viewGroupIds.contains(elementId);
		if (viewId.isEmpty() ? isListedGroup : !isListedGroup){
			continue;
		}

		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_topologyCollectionCompPtr->GetObjectData(elementId, dataPtr)){
			const agentinodata::CTopologyGroup* groupPtr = dynamic_cast<const agentinodata::CTopologyGroup*>(dataPtr.GetPtr());
			if (groupPtr != nullptr){
				TopologyGroupInfo groupInfo;
				groupInfo.name = m_topologyCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
				groupInfo.memberIds = groupPtr->GetMemberIds();

				retVal.insert(elementId, groupInfo);
			}
		}
	}

	return retVal;
}


bool CTopologyControllerComp::SetTopologyGroupMembers(const QByteArray& groupId, const QByteArrayList& memberIds) const
{
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_topologyCollectionCompPtr->GetObjectData(groupId, dataPtr)){
		return false;
	}

	const agentinodata::CTopologyGroup* groupPtr = dynamic_cast<const agentinodata::CTopologyGroup*>(dataPtr.GetPtr());
	if (groupPtr == nullptr){
		return false;
	}

	agentinodata::CTopologyGroup group;
	if (!group.CopyFrom(*groupPtr)){
		return false;
	}

	group.SetMemberIds(memberIds);

	return m_topologyCollectionCompPtr->SetObjectData(groupId, group);
}


QByteArray CTopologyControllerComp::FindParentGroupId(const QByteArray& nodeId, const TopologyGroupMap& groups)
{
	for (TopologyGroupMap::const_iterator groupIter = groups.constBegin(); groupIter != groups.constEnd(); ++groupIter){
		if (groupIter->memberIds.contains(nodeId)){
			return groupIter.key();
		}
	}

	return QByteArray();
}


bool CTopologyControllerComp::IsGroupInside(const QByteArray& groupId, const QByteArray& ancestorGroupId, const TopologyGroupMap& groups)
{
	QSet<QByteArray> visitedGroupIds;

	QByteArray currentGroupId = groupId;
	while (!currentGroupId.isEmpty() && !visitedGroupIds.contains(currentGroupId)){
		if (currentGroupId == ancestorGroupId){
			return true;
		}

		visitedGroupIds.insert(currentGroupId);

		currentGroupId = FindParentGroupId(currentGroupId, groups);
	}

	return false;
}


} // namespace agentinogql


