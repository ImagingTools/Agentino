#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CAgentinoSubscriptionClientComp.h>
#include <agentgql/CServiceSubscriberControllerComp.h>
#include <agentgql/CServiceControllerProxyComp.h>
#include <agentgql/CGetServiceControllerProxyComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CSubscriptionControllerComp.h>
#include <agentinogql/CGqlRepresentationAgentDataComp.h>
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>


/**
	ServiceManagerGqlPck package
*/
namespace AgentinoGqlPck
{

typedef agentgql::CServiceCollectionControllerComp ServiceCollectionController;
typedef agentgql::CServiceControllerComp ServiceController;
typedef agentgql::CAgentinoSubscriptionClientComp AgentinoSubscriptionClient;
typedef agentgql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentgql::CServiceControllerProxyComp ServiceControllerProxy;
typedef agentgql::CGetServiceControllerProxyComp GetServiceControllerProxy;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
typedef icomp::TModelCompWrap<agentinogql::CSubscriptionControllerComp> SubscriptionController;
typedef agentinogql::CGqlRepresentationAgentDataComp GqlRepresentationAgentData;
typedef agentinogql::CTopologyControllerComp TopologyController;
typedef agentinogql::CServiceCollectionSubscriberControllerComp ServiceCollectionSubscriberController;


} // namespace AgentinoGqlPck



