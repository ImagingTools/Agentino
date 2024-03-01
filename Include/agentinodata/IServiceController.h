#pragma once


// Agentino includes
#include <agentinodata/IServiceStatusProvider.h>


#undef StartService

namespace agentinodata
{


/**
	Interface for controller an service.
	\ingroup Service
*/
class IServiceController: virtual public IServiceStatusProvider
{
public:
	virtual bool StartService(const QByteArray& serviceId) = 0;
	virtual bool StopService(const QByteArray& serviceId) = 0;
};


} // namespace agentinodata


