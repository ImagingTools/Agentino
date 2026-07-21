// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "AgentinoDataPck.h"


// ACF includes
#include <icomp/export.h>


namespace AgentinoDataPck
{


I_EXPORT_PACKAGE(
			"AgentinoDataPck",
			"Agentino domain data components",
			IM_PROJECT("Agentino") IM_COMPANY("ImagingTools") "Agentino");

I_EXPORT_COMPONENT(
			ServiceInfo,
			"Service information",
			"Service Information");

I_EXPORT_COMPONENT(
			ServiceCompositeInfo,
			"Service composed information",
			"Service Composed Information");

I_EXPORT_COMPONENT(
			AgentServiceCompositeInfo,
			"Agent local service composed information",
			"Agent Local Service Composed Information");

I_EXPORT_COMPONENT(
			AgentInfo,
			"Agent information",
			"Agent Information");

I_EXPORT_COMPONENT(
			ServiceController,
			"Event-sourced service supervisor (IServiceController)",
			"Agent Service Controller / Supervisor");

I_EXPORT_COMPONENT(
			TerminalController,
			"Remote terminal controller",
			"Remote Terminal Session Controller");

I_EXPORT_COMPONENT(
			LegacyServiceController,
			"Legacy poll-based service controller",
			"Legacy Service Controller");

I_EXPORT_COMPONENT(
			AgentServiceManager,
			"ServiceManager role over agent nested service mirrors (R1.4 split)",
			"Agent Service Manager");

I_EXPORT_COMPONENT(
			ServiceStatusInfo,
			"Service status info",
			"Service Status Info");

I_EXPORT_COMPONENT(
			AgentStatusInfo,
			"Agent status info",
			"Agent Status Info");

I_EXPORT_COMPONENT(
			ProcessHost,
			"Supervised child process host",
			"Process Host");

I_EXPORT_COMPONENT(
			ServiceTypeCatalog,
			"Service-type plugin catalog (load once per type)",
			"Service Type Catalog");


} // namespace AgentinoDataPck


