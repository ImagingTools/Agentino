#pragma once


// ImtCore includes
#include <agentinodata/IService.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


class CService: public CServiceInfo, virtual public IService
{

public:
	CService(ServiceType serviceType = ST_ACF);

	// reimplemented (agentinodata::IService)
	virtual ServiceStatus GetStatus() const override;
	virtual bool StartService() const override;
	virtual bool StopService() const override;

protected:
	ServiceStatus m_status;
};


typedef imtbase::TIdentifiableWrap<CServiceInfo> CServiceInfoIdentifiable;



} // namespace imtauth



