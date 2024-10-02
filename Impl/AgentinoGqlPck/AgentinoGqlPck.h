#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CAgentinoSubscriptionClientComp.h>
#include <agentgql/CServiceLogControllerComp.h>
#include <agentgql/CMessageCollectionControllerComp.h>
#include <agentgql/CAgentMessageCollectionControllerComp.h>
#include <agentgql/CAgentSettingsControllerComp.h>
#include <agentgql/CServiceLogSubscriberControllerComp.h>
#include <agentgql/CAgentGqlRemoteRepresentationControllerComp.h>
#include <agentinogql/CServiceSubscriberControllerComp.h>
#include <agentinogql/CServiceControllerProxyComp.h>
#include <agentinogql/CGetServiceControllerProxyComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CSubscriptionControllerComp.h>
#include <agentinogql/CGqlRepresentationAgentDataComp.h>
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>
#include <agentinogql/CServerServiceCollectionControllerComp.h>
#include <agentinogql/CRemoveServiceControllerProxyComp.h>
#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>
#include <agentinogql/CServiceStatusControllerProxyComp.h>
#include <agentinogql/CAgentConnectionObserverComp.h>
#include <agentinogql/CAgentConnectionSubscriberControllerComp.h>
#include <agentinogql/CAgentsSubscriberProxyControllerComp.h>
#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>


/**
	AgentinoGqlPck package
*/
namespace AgentinoGqlPck
{


typedef agentgql::CServiceCollectionControllerComp ServiceCollectionController;
typedef agentgql::CServiceControllerComp ServiceController;
typedef agentgql::CAgentinoSubscriptionClientComp AgentinoSubscriptionClient;
typedef agentgql::CServiceLogControllerComp ServiceLogController;
typedef agentgql::CMessageCollectionControllerComp MessageCollectionController;
typedef agentgql::CAgentMessageCollectionControllerComp AgentMessageCollectionController;
typedef agentgql::CAgentSettingsControllerComp AgentSettingsController;
typedef agentgql::CServiceLogSubscriberControllerComp ServiceLogSubscriberController;
typedef agentgql::CAgentGqlRemoteRepresentationControllerComp AgentGqlRemoteRepresentationController;
typedef agentinogql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentinogql::CServiceControllerProxyComp ServiceControllerProxy;
typedef agentinogql::CGetServiceControllerProxyComp GetServiceControllerProxy;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
typedef icomp::TModelCompWrap<agentinogql::CSubscriptionControllerComp> SubscriptionController;
typedef agentinogql::CGqlRepresentationAgentDataComp GqlRepresentationAgentData;
typedef agentinogql::CTopologyControllerComp TopologyController;
typedef agentinogql::CServiceCollectionSubscriberControllerComp ServiceCollectionSubscriberController;
typedef agentinogql::CServerServiceCollectionControllerComp ServerServiceCollectionController;
typedef agentinogql::CRemoveServiceControllerProxyComp RemoveServiceControllerProxy;
typedef agentinogql::CServiceStatusCollectionSubscriberControllerComp ServiceStatusCollectionSubscriberController;
typedef agentinogql::CServiceStatusControllerProxyComp ServiceStatusControllerProxy;
typedef agentinogql::CAgentConnectionObserverComp AgentConnectionObserver;
typedef agentinogql::CAgentConnectionSubscriberControllerComp AgentConnectionSubscriberController;
typedef agentinogql::CAgentsSubscriberProxyControllerComp AgentsSubscriberProxyController;
typedef agentinogql::CAgentinoGqlRemoteRepresentationControllerComp AgentinoGqlRemoteRepresentationController;

} // namespace AgentinoGqlPck


