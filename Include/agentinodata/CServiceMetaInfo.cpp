#include <agentinodata/CServiceMetaInfo.h>

// ACF includes
#include <iser/CPrimitiveTypesSerializer.h>
#include <istd/CChangeNotifier.h>
#include <istd/TDelPtr.h>

// ServiceManager includes
#include <agentino/Version.h>


namespace agentinodata
{

// reimplemented (IServiceMetaInfo)
QList<IServiceMetaInfo::Dependency> CServiceMetaInfo::GetDependensies() const
{
	return m_dependencyList;
}


bool CServiceMetaInfo::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = true;

	iser::CArchiveTag elementsTag("Dependencies", "List of dependencies", iser::CArchiveTag::TT_MULTIPLE);
	iser::CArchiveTag elementTag("Dependency", "Single element", iser::CArchiveTag::TT_LEAF, &elementsTag);

	bool isStoring = archive.IsStoring();
	int elementsCount = m_dependencyList.count();

	retVal = retVal && archive.BeginMultiTag(elementsTag, elementTag, elementsCount);
	if (!retVal){
		return false;
	}

	if (isStoring){
		for (int i = 0; i < elementsCount; ++i){
			Dependency dependency = m_dependencyList[i];

			retVal = retVal && archive.BeginTag(elementTag);
			retVal = retVal && archive.Process(dependency.typeId);
			retVal = retVal && archive.EndTag(elementTag);
		}
	}
	else{
		m_dependencyList.clear();

		for (int i = 0; i < elementsCount; ++i){
			Dependency dependency;
			retVal = retVal && archive.BeginTag(elementTag);
			retVal = retVal && archive.Process(dependency.typeId);
			retVal = retVal && archive.EndTag(elementTag);

			if (retVal){
				m_dependencyList.push_back(dependency);
			}
		}
	}

	retVal = retVal && archive.EndTag(elementsTag);

	return retVal;
}


int CServiceMetaInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CServiceMetaInfo::CopyFrom(const IChangeable &object, CompatibilityMode /*mode*/)
{
	const CServiceMetaInfo* sourcePtr = dynamic_cast<const CServiceMetaInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_dependencyList = sourcePtr->m_dependencyList;

		return true;
	}

	return false;
}


istd::IChangeable *CServiceMetaInfo::CloneMe(CompatibilityMode mode) const
{
	istd::TDelPtr<CServiceMetaInfo> clonePtr(new CServiceMetaInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr.PopPtr();
	}

	return nullptr;
}


bool CServiceMetaInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_dependencyList.clear();

	return true;
}


} // namespace agentinodata




