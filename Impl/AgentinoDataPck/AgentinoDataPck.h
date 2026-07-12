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
#include <agentinodata/CAgentCollectionComp.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CSoftwareUpdateInfo.h>
#include <agentinodata/CSoftwareUpdateManager.h>


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
typedef icomp::TModelCompWrap<agentinodata::CServiceControllerComp> ServiceController;
typedef icomp::TModelCompWrap<agentinodata::CAgentCollectionComp> AgentCollection;
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
typedef icomp::TMakeComponentWrap<
					agentinodata::CIdentifiableSoftwareUpdateInfo,
					agentinodata::ISoftwareUpdateInfo,
					iser::ISerializable,
					istd::IChangeable> SoftwareUpdateInfo;
typedef icomp::TMakeComponentWrap<
					agentinodata::CSoftwareUpdateManager,
					agentinodata::ISoftwareUpdateManager> SoftwareUpdateManager;


} // namespace AgentinoDataPck


