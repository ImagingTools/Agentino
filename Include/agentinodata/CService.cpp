#include "agentinodata/CService.h"



namespace agentinodata
{


// public methods

CService::CService(ServiceType serviceType):
			CServiceInfo(serviceType)
{

}


IService::ServiceStatus CService::GetStatus() const
{
	return m_status;
}


bool CService::StartService() const
{
	return false;
}


bool CService::StopService() const
{
	return false;
}


} // namespace imtauth


