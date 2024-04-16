#include "AgentinoDataPck.h"


// ACF includes
#include <icomp/export.h>


namespace AgentinoDataPck
{


I_EXPORT_PACKAGE(
			"ServiceManagerDataPck",
			"ServiceManager's graphql based components",
			IM_PROJECT("ServiceManager") IM_COMPANY("ImagingTools") "Service manager");

I_EXPORT_COMPONENT(
			ServiceInfo,
			"Service information",
			"Service Information");

I_EXPORT_COMPONENT(
			ServiceCompositeInfo,
			"Service composed information",
			"Service Composed Information");

I_EXPORT_COMPONENT(
			AgentInfo,
			"Agent information",
			"Agent Information");

I_EXPORT_COMPONENT(
			ServiceController,
			"ServiceManager agent controller",
			"ServiceManager Agent Service Controller");

I_EXPORT_COMPONENT(
			AgentCollection,
			"Agent collection",
			"Agent Collection");

I_EXPORT_COMPONENT(
			ServiceStatusInfo,
			"Service status info",
			"Service Status Info");

I_EXPORT_COMPONENT(
			AgentStatusInfo,
			"Agent status info",
			"Agent Status Info");


} // namespace AgentinoDataPck


