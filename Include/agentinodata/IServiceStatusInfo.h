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
		SS_NOT_RUNNING
	};

	I_DECLARE_ENUM(ServiceStatus, SS_UNDEFINED, SS_STARTING, SS_STOPPING, SS_RUNNING, SS_NOT_RUNNING);

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


} // namespace agentinodata


