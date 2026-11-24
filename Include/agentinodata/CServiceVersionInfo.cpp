// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceVersionInfo.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>


namespace agentinodata
{


// public methods

CServiceVersionInfo::CServiceVersionInfo()
{
}


void CServiceVersionInfo::SetServiceTypeId(const QByteArray& serviceTypeId)
{
	if (m_serviceTypeId != serviceTypeId){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceTypeId = serviceTypeId;
	}
}


void CServiceVersionInfo::SetOperatingSystem(const QString& operatingSystem)
{
	if (m_operatingSystem != operatingSystem){
		istd::CChangeNotifier changeNotifier(this);

		m_operatingSystem = operatingSystem;
	}
}


void CServiceVersionInfo::SetVersion(const QString& version)
{
	if (m_version != version){
		istd::CChangeNotifier changeNotifier(this);

		m_version = version;
	}
}


void CServiceVersionInfo::SetPath(const QString& path)
{
	if (m_path != path){
		istd::CChangeNotifier changeNotifier(this);

		m_path = path;
	}
}


void CServiceVersionInfo::SetUploadDate(const QDateTime& uploadDate)
{
	if (m_uploadDate != uploadDate){
		istd::CChangeNotifier changeNotifier(this);

		m_uploadDate = uploadDate;
	}
}


// reimplemented (agentinodata::IServiceVersionInfo)

QByteArray CServiceVersionInfo::GetServiceTypeId() const
{
	return m_serviceTypeId;
}


QString CServiceVersionInfo::GetOperatingSystem() const
{
	return m_operatingSystem;
}


QString CServiceVersionInfo::GetVersion() const
{
	return m_version;
}


QString CServiceVersionInfo::GetPath() const
{
	return m_path;
}


QDateTime CServiceVersionInfo::GetUploadDate() const
{
	return m_uploadDate;
}


// reimplemented (iser::ISerializable)

bool CServiceVersionInfo::Serialize(iser::IArchive& archive)
{
	istd::CChangeNotifier changeNotifier(archive.IsStoring() ? nullptr : this, &GetAllChanges());
	Q_UNUSED(changeNotifier);

	bool retVal = true;

	iser::CArchiveTag serviceTypeIdTag("ServiceTypeId", "Service type ID", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceTypeIdTag);
	retVal = retVal && archive.Process(m_serviceTypeId);
	retVal = retVal && archive.EndTag(serviceTypeIdTag);

	iser::CArchiveTag operatingSystemTag("OperatingSystem", "Operating system family", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(operatingSystemTag);
	retVal = retVal && archive.Process(m_operatingSystem);
	retVal = retVal && archive.EndTag(operatingSystemTag);

	iser::CArchiveTag versionTag("Version", "Version", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(versionTag);
	retVal = retVal && archive.Process(m_version);
	retVal = retVal && archive.EndTag(versionTag);

	iser::CArchiveTag pathTag("Path", "Path of the version folder", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(pathTag);
	retVal = retVal && archive.Process(m_path);
	retVal = retVal && archive.EndTag(pathTag);

	iser::CArchiveTag uploadDateTag("UploadDate", "Upload date", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(uploadDateTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_uploadDate);
	retVal = retVal && archive.EndTag(uploadDateTag);

	return retVal;
}


// reimplemented (istd::IChangeable)

int CServiceVersionInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_CLONE | SO_COMPARE | SO_RESET;
}


bool CServiceVersionInfo::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const IServiceVersionInfo* sourcePtr = dynamic_cast<const IServiceVersionInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceTypeId = sourcePtr->GetServiceTypeId();
		m_operatingSystem = sourcePtr->GetOperatingSystem();
		m_version = sourcePtr->GetVersion();
		m_path = sourcePtr->GetPath();
		m_uploadDate = sourcePtr->GetUploadDate();

		return true;
	}

	return false;
}


bool CServiceVersionInfo::IsEqual(const istd::IChangeable& object) const
{
	const IServiceVersionInfo* sourcePtr = dynamic_cast<const IServiceVersionInfo*>(&object);
	if (sourcePtr != nullptr){
		return		(m_serviceTypeId == sourcePtr->GetServiceTypeId()) &&
					(m_operatingSystem == sourcePtr->GetOperatingSystem()) &&
					(m_version == sourcePtr->GetVersion()) &&
					(m_path == sourcePtr->GetPath()) &&
					(m_uploadDate == sourcePtr->GetUploadDate());
	}

	return false;
}


istd::IChangeableUniquePtr CServiceVersionInfo::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CServiceVersionInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CServiceVersionInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_serviceTypeId.clear();
	m_operatingSystem.clear();
	m_version.clear();
	m_path.clear();
	m_uploadDate = QDateTime();

	return true;
}


} // namespace agentinodata


