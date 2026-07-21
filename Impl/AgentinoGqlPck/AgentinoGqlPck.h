// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
<<<<<<< HEAD
#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/TVisualStatusControllerCompWrap.h>
#include <agentgql/TDocumentRevisionControllerCompWrap.h>
#include <agentinogql/CServiceSubscriberControllerComp.h>
#include <agentinogql/CServiceControllerProxyComp.h>
#include <agentinogql/CTerminalControllerProxyComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CSubscriptionControllerComp.h>
#include <agentinogql/CGqlRepresentationAgentDataComp.h>
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>
#include <agentinogql/CServerServiceCollectionControllerComp.h>
#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>
#include <agentinogql/CAgentConnectionObserverComp.h>
#include <agentinogql/CAgentsSubscriberProxyControllerComp.h>
#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>


/**
	AgentinoGqlPck package
*/
namespace AgentinoGqlPck
{


typedef agentinogql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentinogql::CServiceControllerProxyComp ServiceControllerProxy;
typedef agentinogql::CTerminalControllerProxyComp TerminalControllerProxy;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
typedef icomp::TModelCompWrap<agentinogql::CSubscriptionControllerComp> SubscriptionController;
typedef agentinogql::CGqlRepresentationAgentDataComp GqlRepresentationAgentData;
typedef agentinogql::CTopologyControllerComp TopologyController;
typedef agentinogql::CServiceCollectionSubscriberControllerComp ServiceCollectionSubscriberController;
typedef agentinogql::CServerServiceCollectionControllerComp ServerServiceCollectionController;
typedef agentinogql::CServiceStatusCollectionSubscriberControllerComp ServiceStatusCollectionSubscriberController;
typedef agentinogql::CAgentConnectionObserverComp AgentConnectionObserver;
typedef agentinogql::CAgentsSubscriberProxyControllerComp AgentsSubscriberProxyController;
typedef agentinogql::CAgentinoGqlRemoteRepresentationControllerComp AgentinoGqlRemoteRepresentationController;
typedef agentgql::TVisualStatusControllerCompWrap<AgentinoGqlRemoteRepresentationController> AgentinoRemoteVisualStatusController;
typedef agentgql::TDocumentRevisionControllerCompWrap<AgentinoGqlRemoteRepresentationController> AgentinoRemoteDocumentRevisionController;


} // namespace AgentinoGqlPck


=======
#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/TVisualStatusControllerCompWrap.h>
#include <agentgql/TDocumentRevisionControllerCompWrap.h>
#include <agentinogql/CServiceSubscriberControllerComp.h>
#include <agentinogql/CServiceControllerProxyComp.h>
#include <agentinogql/CFileSystemControllerProxyComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CAgentChangeObserverComp.h>
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>
#include <agentinogql/CMirroredServiceCollectionControllerComp.h>
#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>
#include <agentinogql/CAgentsSubscriberProxyControllerComp.h>
#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>
#include <agentinogql/CEnrollmentStoreComp.h>
#include <agentinogql/CEnrollmentGqlControllerComp.h>
#include <agentinogql/CAgentEnrollmentRecord.h>


/**
	AgentinoGqlPck package
*/
namespace AgentinoGqlPck
{


typedef agentinogql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentinogql::CServiceControllerProxyComp ServiceControllerProxy;
typedef agentinogql::CFileSystemControllerProxyComp FileSystemControllerProxy;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
typedef icomp::TModelCompWrap<agentinogql::CAgentChangeObserverComp> AgentChangeObserver;
typedef agentinogql::CTopologyControllerComp TopologyController;
typedef agentinogql::CServiceCollectionSubscriberControllerComp ServiceCollectionSubscriberController;
typedef agentinogql::CMirroredServiceCollectionControllerComp MirroredServiceCollectionController;
typedef agentinogql::CServiceStatusCollectionSubscriberControllerComp ServiceStatusCollectionSubscriberController;
typedef agentinogql::CAgentsSubscriberProxyControllerComp AgentsSubscriberProxyController;
typedef agentinogql::CAgentinoGqlRemoteRepresentationControllerComp AgentinoGqlRemoteRepresentationController;
typedef agentgql::TVisualStatusControllerCompWrap<AgentinoGqlRemoteRepresentationController> AgentinoRemoteVisualStatusController;
typedef agentgql::TDocumentRevisionControllerCompWrap<AgentinoGqlRemoteRepresentationController> AgentinoRemoteDocumentRevisionController;
typedef icomp::TModelCompWrap<agentinogql::CEnrollmentStoreComp> EnrollmentStore;
typedef agentinogql::CEnrollmentGqlControllerComp EnrollmentGqlController;
typedef icomp::TMakeComponentWrap<
					agentinogql::CAgentEnrollmentRecord,
					agentinogql::IAgentEnrollmentRecord,
					iser::ISerializable,
					istd::IChangeable> AgentEnrollmentRecord;


} // namespace AgentinoGqlPck


>>>>>>> origin/main
