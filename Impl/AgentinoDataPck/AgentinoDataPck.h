#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Acula includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceControllerComp.h>


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

typedef icomp::TMakeComponentWrap<
					agentinodata::CIdentifiableAgentInfo,
					agentinodata::IAgentInfo,
					iser::ISerializable,
					istd::IChangeable> AgentInfo;

typedef icomp::TModelCompWrap<agentinodata::CServiceControllerComp> ServiceController;


} // namespace AgentinoDataPck



