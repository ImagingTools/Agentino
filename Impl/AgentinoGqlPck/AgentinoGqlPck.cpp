#include "AgentinoGqlPck.h"


// ACF includes
#include <icomp/export.h>


namespace AgentinoGqlPck
{


I_EXPORT_PACKAGE(
			"AgentinoGqlPck",
			"Agentino's graphql based components",
			IM_PROJECT("Agentino") IM_COMPANY("ImagingTools") "Agentino");

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
			ServiceLogController,
			"Service log controller",
			"Service Log Controller");

I_EXPORT_COMPONENT(
			MessageCollectionController,
			"Message collection controller",
			"Message Collection Controller");

I_EXPORT_COMPONENT(
			AgentSettingsController,
			"Agent settings controller",
			"Agent Settings Controller");

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

I_EXPORT_COMPONENT(
			TopologyController,
			"Topology controller",
			"Topology controller");

I_EXPORT_COMPONENT(
			ServiceCollectionSubscriberController,
			"Service collection subscriber controller",
			"Service Collection Subscriber Controller");

I_EXPORT_COMPONENT(
			ServerServiceCollectionController,
			"Service collection subscriber controller",
			"Service Collection Subscriber Controller");

I_EXPORT_COMPONENT(
			RemoveServiceControllerProxy,
			"Remove service controller proxy",
			"Remove Service Controller Proxy");

I_EXPORT_COMPONENT(
			ServiceStatusCollectionSubscriberController,
			"Service status collection subscriber controller",
			"Service Status Collection Subscriber Controller");

I_EXPORT_COMPONENT(
			ServiceStatusControllerProxy,
			"Service status controller proxy",
			"Service Status Controller Proxy");

I_EXPORT_COMPONENT(
			AgentConnectionObserver,
			"Agent connection observer",
			"Agent Connection Observer");

I_EXPORT_COMPONENT(
			AgentConnectionSubscriberController,
			"Agent connection subscriber controller",
			"Agent Connection Subscriber Controller");


} // namespace AgentinoGqlPck


