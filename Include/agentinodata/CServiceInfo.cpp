#include "agentinodata/CServiceInfo.h"


// Qt includes
#include <QtCore/QByteArrayList>

// ACF includes
#include <istd/TDelPtr.h>
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>
#include <istd/TSingleFactory.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>


namespace agentinodata
{


// public methods

CServiceInfo::CServiceInfo(const QString &typeName, SettingsType settingsType):
	m_settingsType(settingsType),
	m_serviceTypeName(typeName),
	m_isAutoStart(true),
	m_isEnableVerboseMessages(false)
{
	typedef istd::TSingleFactory<istd::IChangeable, imtservice::CUrlConnectionParam> FactoryConnectionImpl;
	m_inputConnections.RegisterFactory<FactoryConnectionImpl>("ConnectionInfo");

	typedef istd::TSingleFactory<istd::IChangeable, imtservice::CUrlConnectionLinkParam> FactoryConnectionLinkImpl;
	m_dependantServiceConnections.RegisterFactory<FactoryConnectionLinkImpl>("ConnectionLink");
}


CServiceInfo::SettingsType CServiceInfo::GetSettingsType() const
{
	return m_settingsType;
}


QString CServiceInfo::GetServiceVersion() const
{
	return m_serviceVersion;
}


QString CServiceInfo::GetServiceTypeName() const
{
	return m_serviceTypeName;
}


QByteArray CServiceInfo::GetServicePath() const
{
	return m_path;
}


void CServiceInfo::SetServicePath(const QByteArray& servicePath)
{
	if (m_path != servicePath){
		istd::CChangeNotifier changeNotifier(this);

		m_path = servicePath;
	}
}


QByteArray CServiceInfo::GetServiceSettingsPath() const
{
	return m_settingsPath;
}


void CServiceInfo::SetServiceSettingsPath(const QByteArray& serviceSettingsPath)
{
	if (m_settingsPath != serviceSettingsPath){
		istd::CChangeNotifier changeNotifier(this);

		m_settingsPath = serviceSettingsPath;
	}
}


QByteArrayList CServiceInfo::GetServiceArguments() const
{
	return m_arguments;
}


void CServiceInfo::SetServiceArguments(const QByteArrayList& serviceArguments)
{
	if (m_arguments != serviceArguments){
		istd::CChangeNotifier changeNotifier(this);

		m_arguments = serviceArguments;
	}
}


void CServiceInfo::SetIsAutoStart(bool isAutoStart)
{
	if (m_isAutoStart != isAutoStart){
		istd::CChangeNotifier changeNotifier(this);

		m_isAutoStart = isAutoStart;
	}
}


void CServiceInfo::SetIsEnableVerboseMessages(bool isEnableVerboseMessages)
{
	if (m_isEnableVerboseMessages != isEnableVerboseMessages){
		istd::CChangeNotifier changeNotifier(this);

		m_isEnableVerboseMessages = isEnableVerboseMessages;
	}
}


void CServiceInfo::SetServiceTypeName(const QByteArray& serviceTypeName)
{
	if (m_serviceTypeName != serviceTypeName){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceTypeName = serviceTypeName;
	}
}


void CServiceInfo::SetServiceVersion(const QString& serviceVersion)
{
	if (m_serviceVersion != serviceVersion){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceVersion = serviceVersion;
	}
}


bool CServiceInfo::IsAutoStart() const
{
	return m_isAutoStart;
}


bool CServiceInfo::IsEnableVerboseMessages() const
{
	return m_isEnableVerboseMessages;
}


imtbase::IObjectCollection* CServiceInfo::GetInputConnections()
{
	return &m_inputConnections;
}


imtbase::IObjectCollection* CServiceInfo::GetDependantServiceConnections()
{
	return &m_dependantServiceConnections;
}


bool CServiceInfo::Serialize(iser::IArchive &archive)
{
	bool retVal = true;

	int settingsType = m_settingsType;

	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 identifiableVersion;
	if (!versionInfo.GetVersionNumber(imtcore::VI_IMTCORE, identifiableVersion)){
		identifiableVersion = 0;
	}

	iser::CArchiveTag idType("Type", "Type", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(idType);
	retVal = retVal && archive.Process(settingsType);
	retVal = retVal && archive.EndTag(idType);

	if (!archive.IsStoring()){
		m_settingsType = (SettingsType)settingsType;
	}

	iser::CArchiveTag serviceTypeNameTag("TypeName", "TypeName", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceTypeNameTag);
	retVal = retVal && archive.Process(m_serviceTypeName);
	retVal = retVal && archive.EndTag(serviceTypeNameTag);

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

	iser::CArchiveTag inputConnectionsTag("InputConnections", "InputConnections", iser::CArchiveTag::TT_GROUP);
	retVal = retVal && archive.BeginTag(inputConnectionsTag);
	retVal = retVal && m_inputConnections.Serialize(archive);
	retVal = retVal && archive.EndTag(inputConnectionsTag);

	iser::CArchiveTag dependantServiceConnectionsTag("DependantServiceConnections", "DependantServiceConnections", iser::CArchiveTag::TT_GROUP);
	retVal = retVal && archive.BeginTag(dependantServiceConnectionsTag);
	retVal = retVal && m_dependantServiceConnections.Serialize(archive);
	retVal = retVal && archive.EndTag(dependantServiceConnectionsTag);

	iser::CArchiveTag versionTag("Version", "Version", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(versionTag);
	retVal = retVal && archive.Process(m_serviceVersion);
	retVal = retVal && archive.EndTag(versionTag);

	if (identifiableVersion > 9681){
		iser::CArchiveTag enableVerboseTag("EnableVerbose", "EnableVerbose", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(enableVerboseTag);
		retVal = retVal && archive.Process(m_isEnableVerboseMessages);
		retVal = retVal && archive.EndTag(enableVerboseTag);
	}

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

		m_settingsType = sourcePtr->m_settingsType;
		m_serviceTypeName = sourcePtr->m_serviceTypeName;
		m_path = sourcePtr->m_path;
		m_settingsPath = sourcePtr->m_settingsPath;
		m_arguments = sourcePtr->m_arguments;
		m_isAutoStart = sourcePtr->m_isAutoStart;
		m_isEnableVerboseMessages =sourcePtr->m_isEnableVerboseMessages;
		m_serviceVersion = sourcePtr->m_serviceVersion;

		m_inputConnections.ResetData();
		m_inputConnections.CopyFrom(sourcePtr->m_inputConnections);

		m_dependantServiceConnections.ResetData();
		m_dependantServiceConnections.CopyFrom(sourcePtr->m_dependantServiceConnections);

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

	m_path.clear();
	m_serviceTypeName.clear();
	m_settingsPath = nullptr;
	m_arguments.clear();
	m_isAutoStart = true;
	m_inputConnections.ResetData();
	m_dependantServiceConnections.ResetData();
	m_settingsType = ST_NONE;

	return true;
}


} // namespace agentinodata


