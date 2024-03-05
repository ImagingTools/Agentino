#include "agentinodata/CServiceStatusInfo.h"


// ACF includes
#include <istd/TDelPtr.h>
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>


namespace agentinodata
{


// public methods

CServiceStatusInfo::CServiceStatusInfo()
	:m_serviceStatus(SS_UNDEFINED)
{
}


CServiceStatusInfo::CServiceStatusInfo(const QByteArray& serviceId, ServiceStatus serviceStatus)
	:m_serviceId(serviceId),
	m_serviceStatus(serviceStatus)
{
}


void CServiceStatusInfo::SetServiceId(const QByteArray& serviceId)
{
	if (m_serviceId != serviceId){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceId = serviceId;
	}
}


void CServiceStatusInfo::SetServiceStatus(IServiceStatusInfo::ServiceStatus status)
{
	if (m_serviceStatus != status){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceStatus = status;
	}
}


// reimplemented (agentinodata::IServiceStatusInfo)

QByteArray CServiceStatusInfo::GetServiceId() const
{
	return m_serviceId;
}


IServiceStatusInfo::ServiceStatus CServiceStatusInfo::GetServiceStatus() const
{
	return m_serviceStatus;
}


bool CServiceStatusInfo::Serialize(iser::IArchive &archive)
{
	bool retVal = true;

	iser::CArchiveTag serviceIdTag("ServiceId", "Service-ID", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceIdTag);
	retVal = retVal && archive.Process(m_serviceId);
	retVal = retVal && archive.EndTag(serviceIdTag);

	iser::CArchiveTag statusTag("ServiceStatus", "Service status", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(statusTag);
	retVal = retVal && I_SERIALIZE_ENUM(ServiceStatus, archive, m_serviceStatus);
	retVal = retVal && archive.EndTag(statusTag);

	return retVal;
}


int CServiceStatusInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CServiceStatusInfo::CopyFrom(const IChangeable &object, CompatibilityMode /*mode*/)
{
	const CServiceStatusInfo* sourcePtr = dynamic_cast<const CServiceStatusInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceId = sourcePtr->m_serviceId;
		m_serviceStatus = sourcePtr->m_serviceStatus;

		return true;
	}

	return false;
}


istd::IChangeable *CServiceStatusInfo::CloneMe(CompatibilityMode mode) const
{
	istd::TDelPtr<CServiceStatusInfo> clonePtr(new CServiceStatusInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr.PopPtr();
	}

	return nullptr;
}


bool CServiceStatusInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_serviceId.clear();
	m_serviceStatus = SS_UNDEFINED;

	return true;
}

} // namespace agentinodata


