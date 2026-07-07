// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "agentinodata/CSoftwareUpdateInfo.h"


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>


namespace agentinodata
{


// public methods

CSoftwareUpdateInfo::CSoftwareUpdateInfo():
	m_updateType(UT_SERVICE),
	m_fileSize(0),
	m_status(US_AVAILABLE)
{
}


void CSoftwareUpdateInfo::SetName(const QString& name)
{
	if (m_name != name){
		istd::CChangeNotifier changeNotifier(this);

		m_name = name;
	}
}


void CSoftwareUpdateInfo::SetVersion(const QString& version)
{
	if (m_version != version){
		istd::CChangeNotifier changeNotifier(this);

		m_version = version;
	}
}


void CSoftwareUpdateInfo::SetUpdateType(UpdateType updateType)
{
	if (m_updateType != updateType){
		istd::CChangeNotifier changeNotifier(this);

		m_updateType = updateType;
	}
}


void CSoftwareUpdateInfo::SetDescription(const QString& description)
{
	if (m_description != description){
		istd::CChangeNotifier changeNotifier(this);

		m_description = description;
	}
}


void CSoftwareUpdateInfo::SetFileSize(qint64 fileSize)
{
	if (m_fileSize != fileSize){
		istd::CChangeNotifier changeNotifier(this);

		m_fileSize = fileSize;
	}
}


void CSoftwareUpdateInfo::SetChecksum(const QString& checksum)
{
	if (m_checksum != checksum){
		istd::CChangeNotifier changeNotifier(this);

		m_checksum = checksum;
	}
}


void CSoftwareUpdateInfo::SetPublishedDate(const QString& publishedDate)
{
	if (m_publishedDate != publishedDate){
		istd::CChangeNotifier changeNotifier(this);

		m_publishedDate = publishedDate;
	}
}


void CSoftwareUpdateInfo::SetStatus(UpdateStatus status)
{
	if (m_status != status){
		istd::CChangeNotifier changeNotifier(this);

		m_status = status;
	}
}


void CSoftwareUpdateInfo::SetChangelog(const QString& changelog)
{
	if (m_changelog != changelog){
		istd::CChangeNotifier changeNotifier(this);

		m_changelog = changelog;
	}
}


void CSoftwareUpdateInfo::SetTargetVersion(const QString& targetVersion)
{
	if (m_targetVersion != targetVersion){
		istd::CChangeNotifier changeNotifier(this);

		m_targetVersion = targetVersion;
	}
}


void CSoftwareUpdateInfo::SetSourceUrl(const QString& sourceUrl)
{
	if (m_sourceUrl != sourceUrl){
		istd::CChangeNotifier changeNotifier(this);

		m_sourceUrl = sourceUrl;
	}
}


// reimplemented (agentinodata::ISoftwareUpdateInfo)

QString CSoftwareUpdateInfo::GetName() const
{
	return m_name;
}


QString CSoftwareUpdateInfo::GetVersion() const
{
	return m_version;
}


CSoftwareUpdateInfo::UpdateType CSoftwareUpdateInfo::GetUpdateType() const
{
	return m_updateType;
}


QString CSoftwareUpdateInfo::GetDescription() const
{
	return m_description;
}


qint64 CSoftwareUpdateInfo::GetFileSize() const
{
	return m_fileSize;
}


QString CSoftwareUpdateInfo::GetChecksum() const
{
	return m_checksum;
}


QString CSoftwareUpdateInfo::GetPublishedDate() const
{
	return m_publishedDate;
}


CSoftwareUpdateInfo::UpdateStatus CSoftwareUpdateInfo::GetStatus() const
{
	return m_status;
}


QString CSoftwareUpdateInfo::GetChangelog() const
{
	return m_changelog;
}


QString CSoftwareUpdateInfo::GetTargetVersion() const
{
	return m_targetVersion;
}


QString CSoftwareUpdateInfo::GetSourceUrl() const
{
	return m_sourceUrl;
}


// reimplemented (iser::ISerializable)

bool CSoftwareUpdateInfo::Serialize(iser::IArchive& archive)
{
	bool retVal = true;

	iser::CArchiveTag nameTag("Name", "Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(nameTag);
	retVal = retVal && archive.Process(m_name);
	retVal = retVal && archive.EndTag(nameTag);

	iser::CArchiveTag versionTag("Version", "Version", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(versionTag);
	retVal = retVal && archive.Process(m_version);
	retVal = retVal && archive.EndTag(versionTag);

	int updateType = m_updateType;
	iser::CArchiveTag updateTypeTag("UpdateType", "UpdateType", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(updateTypeTag);
	retVal = retVal && archive.Process(updateType);
	retVal = retVal && archive.EndTag(updateTypeTag);
	if (!archive.IsStoring()){
		m_updateType = (UpdateType)updateType;
	}

	iser::CArchiveTag descriptionTag("Description", "Description", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(descriptionTag);
	retVal = retVal && archive.Process(m_description);
	retVal = retVal && archive.EndTag(descriptionTag);

	iser::CArchiveTag fileSizeTag("FileSize", "FileSize", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(fileSizeTag);
	retVal = retVal && archive.Process(m_fileSize);
	retVal = retVal && archive.EndTag(fileSizeTag);

	iser::CArchiveTag checksumTag("Checksum", "Checksum", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(checksumTag);
	retVal = retVal && archive.Process(m_checksum);
	retVal = retVal && archive.EndTag(checksumTag);

	iser::CArchiveTag publishedDateTag("PublishedDate", "PublishedDate", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(publishedDateTag);
	retVal = retVal && archive.Process(m_publishedDate);
	retVal = retVal && archive.EndTag(publishedDateTag);

	int status = m_status;
	iser::CArchiveTag statusTag("Status", "Status", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(statusTag);
	retVal = retVal && archive.Process(status);
	retVal = retVal && archive.EndTag(statusTag);
	if (!archive.IsStoring()){
		m_status = (UpdateStatus)status;
	}

	iser::CArchiveTag changelogTag("Changelog", "Changelog", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(changelogTag);
	retVal = retVal && archive.Process(m_changelog);
	retVal = retVal && archive.EndTag(changelogTag);

	iser::CArchiveTag targetVersionTag("TargetVersion", "TargetVersion", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(targetVersionTag);
	retVal = retVal && archive.Process(m_targetVersion);
	retVal = retVal && archive.EndTag(targetVersionTag);

	iser::CArchiveTag sourceUrlTag("SourceUrl", "SourceUrl", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(sourceUrlTag);
	retVal = retVal && archive.Process(m_sourceUrl);
	retVal = retVal && archive.EndTag(sourceUrlTag);

	return retVal;
}


// reimplemented (iser::IChangeable)

int CSoftwareUpdateInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CSoftwareUpdateInfo::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const CSoftwareUpdateInfo* sourcePtr = dynamic_cast<const CSoftwareUpdateInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_name = sourcePtr->m_name;
		m_version = sourcePtr->m_version;
		m_updateType = sourcePtr->m_updateType;
		m_description = sourcePtr->m_description;
		m_fileSize = sourcePtr->m_fileSize;
		m_checksum = sourcePtr->m_checksum;
		m_publishedDate = sourcePtr->m_publishedDate;
		m_status = sourcePtr->m_status;
		m_changelog = sourcePtr->m_changelog;
		m_targetVersion = sourcePtr->m_targetVersion;
		m_sourceUrl = sourcePtr->m_sourceUrl;

		return true;
	}

	return false;
}


istd::IChangeableUniquePtr CSoftwareUpdateInfo::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CSoftwareUpdateInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CSoftwareUpdateInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_name.clear();
	m_version.clear();
	m_updateType = UT_SERVICE;
	m_description.clear();
	m_fileSize = 0;
	m_checksum.clear();
	m_publishedDate.clear();
	m_status = US_AVAILABLE;
	m_changelog.clear();
	m_targetVersion.clear();
	m_sourceUrl.clear();

	return true;
}


} // namespace agentinodata


