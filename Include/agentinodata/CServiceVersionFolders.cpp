// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceVersionFolders.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceVersionFolders)

QString CServiceVersionFolders::GetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem) const
{
	return m_folders.value(FolderKey(serviceTypeId, operatingSystem));
}


void CServiceVersionFolders::SetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem, const QString& path)
{
	const FolderKey key(serviceTypeId, operatingSystem);
	if (path.isEmpty()){
		if (m_folders.contains(key)){
			istd::CChangeNotifier changeNotifier(this);

			m_folders.remove(key);
		}

		return;
	}

	if (m_folders.value(key) != path){
		istd::CChangeNotifier changeNotifier(this);

		m_folders[key] = path;
	}
}


// reimplemented (iser::ISerializable)

bool CServiceVersionFolders::Serialize(iser::IArchive& archive)
{
	istd::CChangeNotifier changeNotifier(archive.IsStoring() ? nullptr : this, &GetAllChanges());
	Q_UNUSED(changeNotifier);

	iser::CArchiveTag foldersTag("Folders", "Folders of the service versions", iser::CArchiveTag::TT_MULTIPLE);
	iser::CArchiveTag folderTag("Folder", "Folder of the versions of a service type for an operating system", iser::CArchiveTag::TT_GROUP, &foldersTag);
	iser::CArchiveTag serviceTypeIdTag("ServiceTypeId", "Service type ID", iser::CArchiveTag::TT_LEAF, &folderTag);
	iser::CArchiveTag operatingSystemTag("OperatingSystem", "Operating system family", iser::CArchiveTag::TT_LEAF, &folderTag);
	iser::CArchiveTag pathTag("Path", "Path of the folder", iser::CArchiveTag::TT_LEAF, &folderTag);

	int foldersCount = m_folders.count();

	bool retVal = archive.BeginMultiTag(foldersTag, folderTag, foldersCount);
	if (!retVal){
		return false;
	}

	if (archive.IsStoring()){
		for (Folders::ConstIterator iter = m_folders.constBegin(); iter != m_folders.constEnd(); ++iter){
			QByteArray serviceTypeId = iter.key().first;
			QString operatingSystem = iter.key().second;
			QString path = iter.value();

			retVal = retVal && archive.BeginTag(folderTag);

			retVal = retVal && archive.BeginTag(serviceTypeIdTag);
			retVal = retVal && archive.Process(serviceTypeId);
			retVal = retVal && archive.EndTag(serviceTypeIdTag);

			retVal = retVal && archive.BeginTag(operatingSystemTag);
			retVal = retVal && archive.Process(operatingSystem);
			retVal = retVal && archive.EndTag(operatingSystemTag);

			retVal = retVal && archive.BeginTag(pathTag);
			retVal = retVal && archive.Process(path);
			retVal = retVal && archive.EndTag(pathTag);

			retVal = retVal && archive.EndTag(folderTag);
		}
	}
	else{
		m_folders.clear();

		for (int i = 0; i < foldersCount; ++i){
			QByteArray serviceTypeId;
			QString operatingSystem;
			QString path;

			retVal = retVal && archive.BeginTag(folderTag);

			retVal = retVal && archive.BeginTag(serviceTypeIdTag);
			retVal = retVal && archive.Process(serviceTypeId);
			retVal = retVal && archive.EndTag(serviceTypeIdTag);

			retVal = retVal && archive.BeginTag(operatingSystemTag);
			retVal = retVal && archive.Process(operatingSystem);
			retVal = retVal && archive.EndTag(operatingSystemTag);

			retVal = retVal && archive.BeginTag(pathTag);
			retVal = retVal && archive.Process(path);
			retVal = retVal && archive.EndTag(pathTag);

			retVal = retVal && archive.EndTag(folderTag);

			if (retVal && !path.isEmpty()){
				m_folders[FolderKey(serviceTypeId, operatingSystem)] = path;
			}
		}
	}

	retVal = retVal && archive.EndTag(foldersTag);

	return retVal;
}


// reimplemented (istd::IChangeable)

int CServiceVersionFolders::GetSupportedOperations() const
{
	return SO_COPY | SO_CLONE | SO_COMPARE | SO_RESET;
}


bool CServiceVersionFolders::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const CServiceVersionFolders* sourcePtr = dynamic_cast<const CServiceVersionFolders*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_folders = sourcePtr->m_folders;

		return true;
	}

	return false;
}


bool CServiceVersionFolders::IsEqual(const istd::IChangeable& object) const
{
	const CServiceVersionFolders* sourcePtr = dynamic_cast<const CServiceVersionFolders*>(&object);
	if (sourcePtr != nullptr){
		return m_folders == sourcePtr->m_folders;
	}

	return false;
}


istd::IChangeableUniquePtr CServiceVersionFolders::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CServiceVersionFolders);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CServiceVersionFolders::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_folders.clear();

	return true;
}


} // namespace agentinodata


