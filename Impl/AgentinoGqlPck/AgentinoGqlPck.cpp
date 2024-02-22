#include "AgentinoGqlPck.h"


// ACF includes
#include <icomp/export.h>


namespace AgentinoGqlPck
{


I_EXPORT_PACKAGE(
			"ServiceManagerGqlPck",
			"ServiceManager's graphql based components",
			IM_PROJECT("ServiceManager") IM_COMPANY("ImagingTools") "Service manager");

I_EXPORT_COMPONENT(
			ServiceCollectionController,
			"Service collection controller",
			"GraphQl Service Collection Controller");

I_EXPORT_COMPONENT(
			ServiceController,
			"Service controller for manage the service",
			"GraphQl Service Controller");

I_EXPORT_COMPONENT(
			AgentinoSubscriptionClient,
			"Service manager subscription client for agent",
			"GraphQl Service  Subscription Client");

I_EXPORT_COMPONENT(
			ServiceSubscriberController,
			"Service subscriber controller",
			"GraphQl Service Subscriber Controller");

I_EXPORT_COMPONENT(
			AgentCollectionController,
			"Agent collection controller",
			"GraphQl Agent Collection Controller");

I_EXPORT_COMPONENT(
			SubscriptionController,
			"Subscription controller for a client",
			"GraphQl Subscription Service Controller");

I_EXPORT_COMPONENT(
			GqlRepresentationAgentData,
			"GrapgQL representation agent data information",
			"GrapgQL Representation Agent Data");

I_EXPORT_COMPONENT(
			ServiceControllerProxy,
			"Service controller proxy",
			"Service Controller Proxy");

I_EXPORT_COMPONENT(
			GetServiceControllerProxy,
			"Get service controller proxy",
			"Get Service Controller Proxy");


} // namespace AgentinoGqlPck


