#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>
//#include <ibase/TMakeModelObserverCompWrap.h>

// ServiceManager includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CAgentinoSubscriptionClientComp.h>
#include <agentgql/CServiceSubscriberControllerComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <agentinogql/CSubscriptionControllerComp.h>
#include <agentinogql/CGqlRepresentationAgentDataComp.h>


/**
	ServiceManagerGqlPck package
*/
namespace AgentinoGqlPck
{

typedef agentgql::CServiceCollectionControllerComp ServiceCollectionController;
typedef agentgql::CServiceControllerComp ServiceController;
typedef agentgql::CAgentinoSubscriptionClientComp AgentinoSubscriptionClient;
typedef agentgql::CServiceSubscriberControllerComp ServiceSubscriberController;
typedef agentinogql::CAgentCollectionControllerComp AgentCollectionController;
//typedef ibase::TMakeModelObserverCompWrap<
//			agentinogql::CSubscriptionControllerComp,
//			imtclientgql::IGqlSubscriptionClient> SubscriptionController;
typedef icomp::TModelCompWrap<agentinogql::CSubscriptionControllerComp> SubscriptionController;
typedef agentinogql::CGqlRepresentationAgentDataComp GqlRepresentationAgentData;


} // namespace AgentinoGqlPck



