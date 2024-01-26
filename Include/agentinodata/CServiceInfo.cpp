#include "agentinodata/CServiceInfo.h"


// Qt includes
#include <QtCore/QByteArrayList>

// ACF includes
#include <istd/TDelPtr.h>
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <agentino/Version.h>


namespace agentinodata
{


// public methods

CServiceInfo::CServiceInfo(ServiceType serviceType):
			m_serviceType(serviceType),
			m_isAutoStart(true)
{

}


CServiceInfo::ServiceType CServiceInfo::GetServiceType() const
{
	return m_serviceType;
}


QString CServiceInfo::GetServiceName() const
{
	return m_name;
}


void CServiceInfo::SetServiceName(const QByteArray& serviceName)
{
	m_name = serviceName;
}


QString CServiceInfo::GetServiceDescription() const
{
	return m_description;
}


void CServiceInfo::SetServiceDescription(const QByteArray& serviceDescription)
{
	m_description = serviceDescription;
}


QByteArray CServiceInfo::GetServicePath() const
{
	return m_path;
}


void CServiceInfo::SetServicePath(const QByteArray& servicePath)
{
	m_path = servicePath;
}


QByteArray CServiceInfo::GetServiceSettingsPath() const
{
	return m_settingsPath;
}


void CServiceInfo::SetServiceSettingsPath(const QByteArray& serviceSettingsPath)
{
	m_settingsPath = serviceSettingsPath;
}


QByteArrayList CServiceInfo::GetServiceArguments() const
{
	return m_arguments;
}


void CServiceInfo::SetServiceArguments(const QByteArrayList& serviceArguments)
{
	m_arguments = serviceArguments;
}


void CServiceInfo::SetIsAutoStart(bool isAutoStart)
{
	m_isAutoStart = isAutoStart;
}


const IServiceMetaInfo* CServiceInfo::GetServiceMetaInfo() const
{
	return &m_serviceMetaInfo;
}


bool CServiceInfo::IsAutoStart() const
{
	return m_isAutoStart;
}


bool CServiceInfo::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = true;

	int serviceType = m_serviceType;

	iser::CArchiveTag idType("Type", "Type", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(idType);
	retVal = retVal && archive.Process(serviceType);
	retVal = retVal && archive.EndTag(idType);

	if (!archive.IsStoring()){
		m_serviceType = (ServiceType)serviceType;
	}

	iser::CArchiveTag nameTag("Name", "Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(nameTag);
	retVal = retVal && archive.Process(m_name);
	retVal = retVal && archive.EndTag(nameTag);

	iser::CArchiveTag descriptionTag("Description", "Description", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(descriptionTag);
	retVal = retVal && archive.Process(m_description);
	retVal = retVal && archive.EndTag(descriptionTag);

	iser::CArchiveTag pathTag("Path", "Path", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(pathTag);
	retVal = retVal && archive.Process(m_path);
	retVal = retVal && archive.EndTag(pathTag);

	iser::CArchiveTag settingsPathTag("SettingsPath", "SettingsPath", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(settingsPathTag);
	retVal = retVal && archive.Process(m_settingsPath);
	retVal = retVal && archive.EndTag(settingsPathTag);

	QByteArray arguments = m_arguments.join(' ');

	iser::CArchiveTag argumentsTag("Arguments", "Arguments", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(argumentsTag);
	retVal = retVal && archive.Process(arguments);
	retVal = retVal && archive.EndTag(argumentsTag);

	if (!archive.IsStoring()){
		m_arguments = arguments.split(' ');
	}

	iser::CArchiveTag autoStartTag("AutoStart", "AutoStart", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(autoStartTag);
	retVal = retVal && archive.Process(m_isAutoStart);
	retVal = retVal && archive.EndTag(autoStartTag);

	return retVal;
}


int CServiceInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CServiceInfo::CopyFrom(const IChangeable &object, CompatibilityMode /*mode*/)
{
	const CServiceInfo* sourcePtr = dynamic_cast<const CServiceInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceType = sourcePtr->m_serviceType;
		m_name = sourcePtr->m_name;
		m_description = sourcePtr->m_description;
		m_path = sourcePtr->m_path;
		m_settingsPath = sourcePtr->m_settingsPath;
		m_arguments = sourcePtr->m_arguments;
		m_isAutoStart = sourcePtr->m_isAutoStart;

		return true;
	}

	return false;
}


istd::IChangeable *CServiceInfo::CloneMe(CompatibilityMode mode) const
{
	istd::TDelPtr<CServiceInfo> clonePtr(new CServiceInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr.PopPtr();
	}

	return nullptr;
}


bool CServiceInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_name.clear();
	m_description.clear();
	m_name.clear();
	m_path.clear();
	m_settingsPath = nullptr;
	m_arguments.clear();
	m_isAutoStart = true;

	return true;
}


} // namespace agentinodata


