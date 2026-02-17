// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
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
			ServiceStatusCollectionSubscriberController,
			"Service status collection subscriber controller",
			"Service Status Collection Subscriber Controller");

I_EXPORT_COMPONENT(
			AgentConnectionObserver,
			"Agent connection observer",
			"Agent Connection Observer");

I_EXPORT_COMPONENT(
			AgentsSubscriberProxyController,
			"Agent subscriber proxy controller",
			"Agent Subscriber Proxy Controller");

I_EXPORT_COMPONENT(
			AgentinoGqlRemoteRepresentationController,
			"Retranslate requests from argentina to agent",
			"Agentino Retranslate Request Controller");

I_EXPORT_COMPONENT(
			AgentinoRemoteVisualStatusController,
			"Agentino remote visual status controller",
			"Agentino Remote Visual Status Controller");

I_EXPORT_COMPONENT(
			AgentinoRemoteDocumentRevisionController,
			"Agentino remote document revision controller",
			"Agentino Remote Document Revision Controller");


} // namespace AgentinoGqlPck


