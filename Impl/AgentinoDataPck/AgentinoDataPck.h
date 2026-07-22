// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <agentinodata/CAgentServiceCompositeInfoComp.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceControllerComp.h>
#include <agentinodata/CTerminalSessionManagerComp.h>
#include <agentinodata/CAgentServiceManagerComp.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CProcessHostComp.h>
#include <agentinodata/CServiceSupervisorComp.h>
#include <agentinodata/CServiceTypeCatalogComp.h>


/**
	AgentinoGqlPck package
*/
namespace AgentinoDataPck
{


typedef icomp::TMakeComponentWrap<
					agentinodata::CIdentifiableServiceInfo,
					agentinodata::IServiceInfo,
					iser::ISerializable,
					istd::IChangeable> ServiceInfo;
typedef icomp::TModelCompWrap<agentinodata::CServiceCompositeInfoComp> ServiceCompositeInfo;
typedef icomp::TModelCompWrap<agentinodata::CAgentServiceCompositeInfoComp> AgentServiceCompositeInfo;
typedef icomp::TMakeComponentWrap<
					agentinodata::CIdentifiableAgentInfo,
					agentinodata::IAgentInfo,
					iser::ISerializable,
					istd::IChangeable> AgentInfo;
// ServiceController is the event-sourced supervisor (Architecture Audit §4.6 cutover).
typedef icomp::TModelCompWrap<agentinodata::CServiceSupervisorComp> ServiceController;
typedef icomp::TModelCompWrap<agentinodata::CServiceControllerComp> LegacyServiceController;
typedef icomp::TModelCompWrap<agentinodata::CTerminalSessionManagerComp> TerminalSessionManager;
typedef icomp::TModelCompWrap<agentinodata::CAgentServiceManagerComp> AgentServiceManager;
typedef icomp::TMakeComponentWrap<
					agentinodata::CServiceStatusInfo,
					agentinodata::IServiceStatusInfo,
					iser::ISerializable,
					istd::IChangeable> ServiceStatusInfo;
typedef icomp::TMakeComponentWrap<
					agentinodata::CAgentStatusInfo,
					agentinodata::IAgentStatusInfo,
					iser::ISerializable,
					istd::IChangeable> AgentStatusInfo;
typedef icomp::TModelCompWrap<agentinodata::CProcessHostComp> ProcessHost;
typedef icomp::TModelCompWrap<agentinodata::CServiceTypeCatalogComp> ServiceTypeCatalog;


} // namespace AgentinoDataPck

