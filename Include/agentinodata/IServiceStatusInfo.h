// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <iser/IObject.h>

// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentinodata
{


/**
	Interface for describing a service status info.
	\ingroup Service
*/
class IServiceStatusInfo: virtual public iser::IObject
{
public:
	enum ServiceStatus
	{
		SS_UNDEFINED,
		SS_STARTING,
		SS_STOPPING,
		SS_RUNNING,
		SS_NOT_RUNNING,
		SS_RUNNING_IMPOSSIBLE
	};

	I_DECLARE_ENUM(ServiceStatus, SS_UNDEFINED, SS_STARTING, SS_STOPPING, SS_RUNNING, SS_NOT_RUNNING, SS_RUNNING_IMPOSSIBLE);

	/**
		Get ID of the service.
	*/
	virtual QByteArray GetServiceId() const = 0;

	/**
		Get status of the service.
	*/
	virtual ServiceStatus GetServiceStatus() const = 0;
};


ProcessStateEnum GetProcceStateRepresentation(IServiceStatusInfo::ServiceStatus processState);

/**
	Inverse of GetProcceStateRepresentation: parse a wire status back into the enum.

	Accepts the ProcessStateEnum ids emitted by the agent ("running", "notRunning",
	"starting", "stopping", "undefined"), the uppercase wire form used by the GQL
	subscription payloads ("RUNNING", "NOT_RUNNING", ...) and the I_DECLARE_ENUM
	names ("SS_RUNNING", ...). Returns false when nothing matches, leaving
	\a processState untouched.
*/
bool GetServiceStatusFromRepresentation(
			const QString& representation,
			IServiceStatusInfo::ServiceStatus& processState);


} // namespace agentinodata


