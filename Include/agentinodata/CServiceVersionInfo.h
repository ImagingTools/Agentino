// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/IServiceVersionInfo.h>


namespace agentinodata
{


class CServiceVersionInfo: virtual public IServiceVersionInfo
{
public:
	CServiceVersionInfo();

	virtual void SetServiceTypeId(const QByteArray& serviceTypeId);
	virtual void SetOperatingSystem(const QString& operatingSystem);
	virtual void SetVersion(const QString& version);
	virtual void SetPath(const QString& path);
	virtual void SetUploadDate(const QDateTime& uploadDate);

	// reimplemented (agentinodata::IServiceVersionInfo)
	virtual QByteArray GetServiceTypeId() const override;
	virtual QString GetOperatingSystem() const override;
	virtual QString GetVersion() const override;
	virtual QString GetPath() const override;
	virtual QDateTime GetUploadDate() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual bool IsEqual(const istd::IChangeable& object) const override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QByteArray m_serviceTypeId;
	QString m_operatingSystem;
	QString m_version;
	QString m_path;
	QDateTime m_uploadDate;
};


} // namespace agentinodata


