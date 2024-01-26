#pragma once


// ImtCore includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/CServiceMetaInfo.h>
#include <imtbase/TIdentifiableWrap.h>


namespace agentinodata
{


class CServiceInfo: virtual public IServiceInfo
{

public:
	CServiceInfo(ServiceType serviceType = ST_ACF);
	virtual void SetServiceName(const QByteArray& serviceName);
	virtual void SetServiceDescription(const QByteArray& serviceName);
	virtual void SetServicePath(const QByteArray& servicePath);
	virtual void SetServiceSettingsPath(const QByteArray& serviceSettingsPath);
	virtual void SetServiceArguments(const QByteArrayList &serviceArguments);
	virtual void SetIsAutoStart(bool isAutoStart);

	// reimplemented (agentinodata::IServiceInfo)
	virtual ServiceType GetServiceType() const override;
//	virtual void SetServiceType(const ServiceType serviceType) override;
	virtual QString GetServiceName() const override;
	virtual QString GetServiceDescription() const override;
	virtual QByteArray GetServicePath() const override;
	virtual QByteArray GetServiceSettingsPath() const override;
	virtual QByteArrayList GetServiceArguments() const override;
	virtual const IServiceMetaInfo* GetServiceMetaInfo() const override;
	virtual bool IsAutoStart() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeable *CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	ServiceType m_serviceType;
	QString m_name;
//	QByteArray m_id;
	QString m_description;
	QByteArray m_path;
	QByteArray m_settingsPath;
	QByteArrayList m_arguments;
	CServiceMetaInfo m_serviceMetaInfo;
	bool m_isAutoStart;
};


typedef imtbase::TIdentifiableWrap<CServiceInfo> CIdentifiableServiceInfo;


} // namespace agentinodata



