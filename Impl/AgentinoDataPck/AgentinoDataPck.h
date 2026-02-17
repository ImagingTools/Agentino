// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceControllerComp.h>
#include <agentinodata/CAgentCollectionComp.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>


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


} // namespace AgentinoDataPck


