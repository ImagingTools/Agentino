// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "AgentinoGqlPck.h"


// ACF includes
#include <icomp/export.h>

// ImtCore includes
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/DocumentRevision.h>


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
			AgentChangeObserver,
			"The single server-side observer of agent changes (status, collection, connection)",
			"Agent Change Observer");

I_EXPORT_COMPONENT(
			ServiceControllerProxy,
			"Service controller proxy",
			"Service Controller Proxy");

I_EXPORT_COMPONENT(
			TerminalControllerProxy,
			"Remote terminal controller proxy",
			"Remote Terminal Controller Proxy");

I_EXPORT_COMPONENT(
			FileSystemControllerProxy,
			"File system controller proxy forwarding browse requests to an agent by clientid",
			"File System Controller Proxy Folder Browser");

I_EXPORT_COMPONENT(
			TopologyController,
			"Topology controller",
			"Topology controller");

I_EXPORT_COMPONENT(
			ServiceCollectionSubscriberController,
			"Service collection subscriber controller",
			"Service Collection Subscriber Controller");

I_EXPORT_COMPONENT(
			MirroredServiceCollectionController,
			"Read surface over the server-side mirror of an agent's services",
			"Mirrored Service Collection Controller");

I_EXPORT_COMPONENT(
			ServiceStatusCollectionSubscriberController,
			"Service status collection subscriber controller",
			"Service Status Collection Subscriber Controller");

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

I_EXPORT_COMPONENT(
			EnrollmentStore,
			"Durable agent enrollment store, gate and controller",
			"Agent Enrollment Store");

I_EXPORT_COMPONENT(
			EnrollmentGqlController,
			"GraphQL enrollment admin (approve/reject/revoke/list)",
			"Enrollment GraphQL Controller");

I_EXPORT_COMPONENT(
			AgentEnrollmentRecord,
			"One agent's enrollment record (EnrollmentStore's RecordCollection item)",
			"Agent Enrollment Record");


} // namespace AgentinoGqlPck


