#pragma once


// ACF includes
#include <iprm/IParamsSet.h>

// ServiceManager includes
#include <agentinodata/IServiceController.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceMetaInfo.h>
#include <agentinodata/IAgentInfo.h>


namespace agentinodata
{


/**
	Interface for describing an agent.
	\ingroup Service
*/
class IServiceManager: virtual public IServiceController, virtual public IAgentInfo
{
public:
	virtual const int GetServicesCount() const = 0;
	virtual const IServiceInfo* GetServiceInfo(const QByteArray& serviceId) const = 0;
	virtual const iprm::IParamsSet* GetServiceConfiguration(const QByteArray& serviceId) const = 0;
	virtual const IServiceMetaInfo* GetServiceMetaInfo(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata




