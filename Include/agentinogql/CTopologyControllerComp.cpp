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
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinogql
{


// Type-ID of user-defined groups in the topology collection (see Repositories.acc).
static const QByteArray s_topologyGroupTypeId = QByteArrayLiteral("TopologyGroup");


// Layout (x/y) is a view concern stored only in TopologyCollection on the server/GUI
// side — it is never pushed to agents or included in the agent↔server domain event stream
// (Architecture Audit §4.9 / Step 7).

// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CTopology CTopologyControllerComp::OnGetTopology(
			const sdl::V1_0::agentino::CGetTopologyGqlRequest& /*getTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CTopology response;
	
	
	QList<sdl::V1_0::agentino::CService> services;

	// Agents are the default composite nodes: a service belongs to its agent unless
	// it was moved into a user-defined group.
	const TopologyGroupMap groups = GetTopologyGroups();
		
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
				
				bool agentOnline = IsAgentOnline(elementId);

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
					
					sdl::V1_0::agentino::CService service;
					
					service.id = serviceElementId;
					service.agentId = elementId;
					service.nodeType = sdl::V1_0::agentino::TopologyNodeType::SERVICE;
					service.isComposite = false;

					const QByteArray parentGroupId = FindParentGroupId(serviceElementId, groups);
					service.parentId = parentGroupId.isEmpty() ? elementId : parentGroupId;
					
					QPoint point = GetServiceCoordinate(serviceElementId);
					
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
					
					services << service;
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
		if (!SetServiceCoordinate(id, point)){
			errorMessage = QString("Unable to save topology. Error: Failed to store coordinates for '%1'")
						.arg(QString::fromUtf8(id));
			SendErrorMessage(0, errorMessage, "CTopologyControllerComp");
			return response;
		}
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

	const TopologyGroupMap groups = GetTopologyGroups();
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

	const QByteArray groupId = m_topologyCollectionCompPtr->InsertNewObject(s_topologyGroupTypeId, name, "", &group);
	if (groupId.isEmpty()){
		errorMessage = QString("Unable to create topology group. Error: Failed to insert group '%1'").arg(name);
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
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

	const TopologyGroupMap groups = GetTopologyGroups();
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

	const TopologyGroupMap groups = GetTopologyGroups();
	if (!groups.contains(groupId)){
		errorMessage = QString("Unable to remove topology group. Error: Group '%1' does not exist").arg(QString::fromUtf8(groupId));
		SendErrorMessage(0, errorMessage, "CTopologyControllerComp");

		return response;
	}

	istd::CChangeGroup changeGroup(m_topologyCollectionCompPtr.GetPtr());

	// The members of the removed group are moved one level up. Members of a root group
	// fall back to their default place (services to their agents, agents and groups to root).
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


CTopologyControllerComp::TopologyGroupMap CTopologyControllerComp::GetTopologyGroups() const
{
	TopologyGroupMap retVal;

	if (!m_topologyCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'TopologyCollection' was not set", "CTopologyControllerComp");
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids elementIds = m_topologyCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
		if (m_topologyCollectionCompPtr->GetObjectTypeId(elementId) != s_topologyGroupTypeId){
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


