// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "agentinodata/CServiceInfo.h"


// Qt includes
#include <QtCore/QByteArrayList>

// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <istd/TSingleFactory.h>

// ImtCore includes
#include <imtcore/Version.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>


namespace agentinodata
{


// public methods

CServiceInfo::CServiceInfo(const QString &typeName, SettingsType settingsType):
	m_settingsType(settingsType),
	m_serviceTypeId(typeName),
	m_tracingLevel(-1),
	m_isAutoStart(true)
{
	typedef istd::TSingleFactory<istd::IChangeable, imtservice::CUrlConnectionParam> FactoryConnectionImpl;
	m_inputConnections.RegisterFactory<FactoryConnectionImpl>("ConnectionInfo");

	typedef istd::TSingleFactory<istd::IChangeable, imtservice::CUrlConnectionLinkParam> FactoryConnectionLinkImpl;
	m_dependantServiceConnections.RegisterFactory<FactoryConnectionLinkImpl>("ConnectionLink");
}


void CServiceInfo::SetServiceName(const QString& name)
{
	if (m_serviceName != name){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceName = name;
	}
}


void CServiceInfo::SetServiceDescription(const QString& description)
{
	if (m_serviceDescription != description){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceDescription = description;
	}
}


void CServiceInfo::SetServicePath(const QByteArray& servicePath)
{
	if (m_path != servicePath){
		istd::CChangeNotifier changeNotifier(this);

		m_path = servicePath;
	}
}


void CServiceInfo::SetServiceSettingsPath(const QByteArray& serviceSettingsPath)
{
	if (m_settingsPath != serviceSettingsPath){
		istd::CChangeNotifier changeNotifier(this);

		m_settingsPath = serviceSettingsPath;
	}
}


void CServiceInfo::SetStartScriptPath(const QByteArray& startScriptPath)
{
	m_startScriptPath = startScriptPath;
}


void CServiceInfo::SetStopScriptPath(const QByteArray& stopScriptPath)
{
	m_stopScriptPath = stopScriptPath;
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


void CServiceInfo::SetServiceTypeId(const QByteArray& serviceTypeName)
{
	if (m_serviceTypeId != serviceTypeName){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceTypeId = serviceTypeName;
	}
}


void CServiceInfo::SetServiceVersion(const QString& serviceVersion)
{
	if (m_serviceVersion != serviceVersion){
		istd::CChangeNotifier changeNotifier(this);

		m_serviceVersion = serviceVersion;
	}
}


QString CServiceInfo::GetServiceName() const
{
	return m_serviceName;
}


QString CServiceInfo::GetServiceDescription() const
{
	return m_serviceDescription;
}


CServiceInfo::SettingsType CServiceInfo::GetSettingsType() const
{
	return m_settingsType;
}


QString CServiceInfo::GetServiceVersion() const
{
	return m_serviceVersion;
}


QString CServiceInfo::GetServiceTypeId() const
{
	return m_serviceTypeId;
}


QByteArray CServiceInfo::GetServicePath() const
{
	return m_path;
}


QByteArray CServiceInfo::GetServiceSettingsPath() const
{
	return m_settingsPath;
}


QByteArrayList CServiceInfo::GetServiceArguments() const
{
	return m_arguments;
}


bool CServiceInfo::IsAutoStart() const
{
	return m_isAutoStart;
}


QByteArray CServiceInfo::GetStartScriptPath() const
{
	return m_startScriptPath;
}


QByteArray CServiceInfo::GetStopScriptPath() const
{
	return m_stopScriptPath;
}


imtbase::IObjectCollection* CServiceInfo::GetInputConnections()
{
	return &m_inputConnections;
}


imtbase::IObjectCollection* CServiceInfo::GetDependantServiceConnections()
{
	return &m_dependantServiceConnections;
}


// reimplemented (ilog::ITracingConfiguration)

int CServiceInfo::GetTracingLevel() const
{
	return m_tracingLevel;
}


void CServiceInfo::SetTracingLevel(int tracingLevel)
{
	if (m_tracingLevel != tracingLevel){
		istd::CChangeNotifier changeNotifier(this);

		m_tracingLevel = tracingLevel;
	}
}


// reimplemented (iser::ISerializable)

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
	retVal = retVal && archive.Process(m_serviceTypeId);
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

	if (identifiableVersion < 9897){
		iser::CArchiveTag enableVerboseTag("EnableVerbose", "EnableVerbose", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(enableVerboseTag);
		int isEnableVerboseMessages = 0;
		retVal = retVal && archive.Process(isEnableVerboseMessages);
		retVal = retVal && archive.EndTag(enableVerboseTag);
	}
	if (identifiableVersion >= 9897){
		iser::CArchiveTag startScriptPathTag("StartScriptPath", "StartScriptPath", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(startScriptPathTag);
		retVal = retVal && archive.Process(m_startScriptPath);
		retVal = retVal && archive.EndTag(startScriptPathTag);

		iser::CArchiveTag stopScriptPathTag("StopScriptPath", "StopScriptPath", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(stopScriptPathTag);
		retVal = retVal && archive.Process(m_stopScriptPath);
		retVal = retVal && archive.EndTag(stopScriptPathTag);

		iser::CArchiveTag tracingLevelTag("TracingLevel", "TracingLevel", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(tracingLevelTag);
		retVal = retVal && archive.Process(m_tracingLevel);
		retVal = retVal && archive.EndTag(tracingLevelTag);
	}
	if (identifiableVersion >= 12968){
		iser::CArchiveTag nameTag("Name", "Service Name", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(nameTag);
		retVal = retVal && archive.Process(m_serviceName);
		retVal = retVal && archive.EndTag(nameTag);

		iser::CArchiveTag descriptionTag("Description", "Service Description", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(descriptionTag);
		retVal = retVal && archive.Process(m_serviceDescription);
		retVal = retVal && archive.EndTag(descriptionTag);
	}

	return retVal;
}


// reimplemented (iser::IChangeable)

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
		m_serviceName = sourcePtr->m_serviceName;
		m_serviceDescription = sourcePtr->m_serviceDescription;
		m_serviceTypeId = sourcePtr->m_serviceTypeId;
		m_path = sourcePtr->m_path;
		m_startScriptPath =sourcePtr->m_startScriptPath;
		m_stopScriptPath =sourcePtr->m_stopScriptPath;
		m_settingsPath = sourcePtr->m_settingsPath;
		m_arguments = sourcePtr->m_arguments;
		m_isAutoStart = sourcePtr->m_isAutoStart;
		m_tracingLevel =sourcePtr->m_tracingLevel;
		m_serviceVersion = sourcePtr->m_serviceVersion;

		m_inputConnections.ResetData();
		m_inputConnections.CopyFrom(sourcePtr->m_inputConnections);

		m_dependantServiceConnections.ResetData();
		m_dependantServiceConnections.CopyFrom(sourcePtr->m_dependantServiceConnections);

		return true;
	}

	return false;
}


istd::IChangeableUniquePtr CServiceInfo::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CServiceInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CServiceInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_path.clear();
	m_serviceName.clear();
	m_serviceDescription.clear();
	m_serviceTypeId.clear();
	m_settingsPath = nullptr;
	m_arguments.clear();
	m_isAutoStart = true;
	m_inputConnections.ResetData();
	m_dependantServiceConnections.ResetData();
	m_settingsType = ST_NONE;

	return true;
}


} // namespace agentinodata


