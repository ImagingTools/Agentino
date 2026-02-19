// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


class CServiceStatusInfo: virtual public IServiceStatusInfo
{
public:
	CServiceStatusInfo();
	CServiceStatusInfo(const QByteArray& serviceId, ServiceStatus serviceStatus);

	virtual void SetServiceId(const QByteArray& serviceId);
	virtual void SetServiceStatus(ServiceStatus status);

	// reimplemented (agentinodata::IServiceStatusInfo)
	virtual QByteArray GetServiceId() const override;
	virtual ServiceStatus GetServiceStatus() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QByteArray m_serviceId;
	ServiceStatus m_serviceStatus;
};


} // namespace agentinodata



