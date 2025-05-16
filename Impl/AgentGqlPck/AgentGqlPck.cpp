#include "AgentGqlPck.h"


// ACF includes
#include <icomp/export.h>


namespace AgentGqlPck
{


I_EXPORT_PACKAGE(
			"AgentGqlPck",
			"Agent's graphql based components",
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
			AgentMessageCollectionController,
			"Agent message collection controller",
			"Agent Message Collection Controller");

I_EXPORT_COMPONENT(
			AgentSettingsController,
			"Agent settings controller",
			"Agent Settings Controller");

I_EXPORT_COMPONENT(
			ServiceLogSubscriberController,
			"Service log subscriber controller",
			"Service Log Subscriber Controller");

I_EXPORT_COMPONENT(
			AgentGqlRemoteRepresentationController,
			"Retranslate requests from argent to servicies",
			"Agentino Retranslate Request Controller");

I_EXPORT_COMPONENT(
			AgentConnectionSubscriberController,
			"Agent connection subscriber controller",
			"Agent Connection Subscriber Controller");

I_EXPORT_COMPONENT(
			AgentServicesRemoteSubscriberProxy,
			"Agent services remote subscriber controller",
			"Agent Services Remote Subscriber Controller");

I_EXPORT_COMPONENT(
			AgentRemoteVisualStatusController,
			"Agent remote visual status controller",
			"Agent Remote Visual Status Controller");

I_EXPORT_COMPONENT(
			AgentRemoteDocumentRevisionController,
			"Agent remote document revision controller",
			"Agent Remote Document Revision Controller");


} // namespace AgentGqlPck


