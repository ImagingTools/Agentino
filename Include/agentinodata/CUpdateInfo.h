// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/TIdentifiableWrap.h>

// Agentino includes
#include <agentinodata/IUpdateInfo.h>


namespace agentinodata
{


class CUpdateInfo: virtual public IUpdateInfo
{
public:
	CUpdateInfo();

	void SetName(const QString& name);
	void SetVersion(const QString& version);
	void SetUpdateType(UpdateType updateType);
	void SetDescription(const QString& description);
	void SetFileSize(qint64 fileSize);
	void SetChecksum(const QString& checksum);
	void SetPublishedDate(const QString& publishedDate);
	void SetStatus(UpdateStatus status);
	void SetChangelog(const QString& changelog);
	void SetTargetVersion(const QString& targetVersion);
	void SetSourceUrl(const QString& sourceUrl);

	// reimplemented (agentinodata::IUpdateInfo)
	virtual QString GetName() const override;
	virtual QString GetVersion() const override;
	virtual UpdateType GetUpdateType() const override;
	virtual QString GetDescription() const override;
	virtual qint64 GetFileSize() const override;
	virtual QString GetChecksum() const override;
	virtual QString GetPublishedDate() const override;
	virtual UpdateStatus GetStatus() const override;
	virtual QString GetChangelog() const override;
	virtual QString GetTargetVersion() const override;
	virtual QString GetSourceUrl() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QString m_name;
	QString m_version;
	UpdateType m_updateType;
	QString m_description;
	qint64 m_fileSize;
	QString m_checksum;
	QString m_publishedDate;
	UpdateStatus m_status;
	QString m_changelog;
	QString m_targetVersion;
	QString m_sourceUrl;
};


typedef imtbase::TIdentifiableWrap<CUpdateInfo> CIdentifiableUpdateInfo;


} // namespace agentinodata


