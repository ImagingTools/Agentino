#pragma once

// Acf includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


/**
	Interface for describing an service.
	\ingroup Service
*/
class IService:
		virtual public IServiceInfo
{
public:
	enum ServiceStatus
	{
		SS_STOPPED,

		SS_RUNNING
	};

	virtual ServiceStatus GetStatus() const = 0;
	virtual bool StartService() const = 0;
	virtual bool StopService() const = 0;
};


} // namespace agentinodata


