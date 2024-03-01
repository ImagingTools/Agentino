#pragma once


// ACF includes
#include <iser/IObject.h>


namespace agentinodata
{


/**
	Interface for describing an service status info.
	\ingroup Service
*/
class IServiceStatusInfo: virtual public iser::IObject
{
public:
	enum ServiceStatus
	{
		SS_NONE,
		SS_RUNNING,
		SS_NOT_RUNNING
	};

	I_DECLARE_ENUM(ServiceStatus, SS_NONE, SS_RUNNING, SS_NOT_RUNNING);

	/**
		Get ID of the service.
	*/
	virtual QByteArray GetServiceId() const = 0;

	/**
		Get ID of the service.
	*/
	virtual ServiceStatus GetServiceStatus() const = 0;
};


} // namespace agentinodata


