#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CAgentinoSubscriptionClientComp.h>
#include <agentgql/CServiceLogControllerComp.h>
#include <agentgql/CAgentSettingsControllerComp.h>
#include <agentinogql/CServiceSubscriberControllerComp.h>
#include <agentinogql/CServiceControllerProxyComp.h>
#include <agentinogql/CGetServiceControllerProxyComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CSubscriptionControllerComp.h>
#include <agentinogql/CGqlRepresentationAgentDataComp.h>
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>
#include <agentinogql/CServiceCollectionControllerComp.h>
#include <agentinogql/CRemoveServiceControllerProxyComp.h>
#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>
#include <agentinogql/CServiceStatusControllerProxyComp.h>
#include <agentinogql/CAgentConnectionObserverComp.h>


/**
	AgentinoGqlPck package
*/
namespace AgentinoGqlPck
{


typedef agentgql::CServiceCollectionControllerComp ServiceCollectionController;
typedef agentgql::CServiceControllerComp ServiceController;
typedef agentgql::CAgentinoSubscriptionClientComp AgentinoSubscriptionClient;
typedef agentgql::CServiceLogControllerComp ServiceLogController;
typedef agentgql::CAgentSettingsControllerComp AgentSettingsController;
typedef agentinogql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentinogql::CServiceControllerProxyComp ServiceControllerProxy;
typedef agentinogql::CGetServiceControllerProxyComp GetServiceControllerProxy;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
typedef icomp::TModelCompWrap<agentinogql::CSubscriptionControllerComp> SubscriptionController;
typedef agentinogql::CGqlRepresentationAgentDataComp GqlRepresentationAgentData;
typedef agentinogql::CTopologyControllerComp TopologyController;
typedef agentinogql::CServiceCollectionSubscriberControllerComp ServiceCollectionSubscriberController;
typedef agentinogql::CServiceCollectionControllerComp ServerServiceCollectionController;
typedef agentinogql::CRemoveServiceControllerProxyComp RemoveServiceControllerProxy;
typedef agentinogql::CServiceStatusCollectionSubscriberControllerComp ServiceStatusCollectionSubscriberController;
typedef agentinogql::CServiceStatusControllerProxyComp ServiceStatusControllerProxy;
typedef agentinogql::CAgentConnectionObserverComp AgentConnectionObserver;


} // namespace AgentinoGqlPck


