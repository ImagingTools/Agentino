// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#include <agentinodata/CAgentInfo.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>
#include <istd/TSingleFactory.h>

// ImtCore includes
#include <imtcore/Version.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


// public methods

CAgentInfo::CAgentInfo():
	m_tracingLevel(-1),
	m_modelUpdateBridge(this, imod::CModelUpdateBridge::UF_SOURCE)
{
	typedef istd::TSingleFactory<istd::IChangeable, agentinodata::CIdentifiableServiceInfo> FactoryServiceImpl;
	m_serviceCollection.RegisterFactory<FactoryServiceImpl>("ServiceInfo");

	m_serviceCollection.AttachObserver(&m_modelUpdateBridge);
}


void CAgentInfo::SetLastConnection(const QDateTime& lastConnection)
{
	if (m_lastConnection != lastConnection){
		istd::CChangeNotifier changeNotifier(this);

		m_lastConnection = lastConnection;
	}
}


void CAgentInfo::SetVersion(const QString& version)
{
	m_version = version;
}


void CAgentInfo::SetComputerName(const QString& computerName)
{
	if (m_computerName != computerName){
		istd::CChangeNotifier changeNotifier(this);

		m_computerName = computerName;
	}
}


// reimplemented (agentinodata::IAgentInfo)

QString CAgentInfo::GetVersion() const
{
	return m_version;
}

QDateTime CAgentInfo::GetLastConnection() const
{
	return m_lastConnection;
}


QString CAgentInfo::GetComputerName() const
{
	return m_computerName;
}


imtbase::IObjectCollection* CAgentInfo::GetServiceCollection()
{
	return &m_serviceCollection;
}


// reimplemented (ilog::ITracingConfiguration)

int CAgentInfo::GetTracingLevel() const
{
	return m_tracingLevel;
}


void CAgentInfo::SetTracingLevel(int tracingLevel)
{
	if (m_tracingLevel != tracingLevel){
		istd::CChangeNotifier changeNotifier(this);

		m_tracingLevel = tracingLevel;
	}
}


// reimplemented (iser::ISerializable)

bool CAgentInfo::Serialize(iser::IArchive &archive)
{
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 imtCoreVersion;
	if (!versionInfo.GetVersionNumber(imtcore::VI_IMTCORE, imtCoreVersion)){
		imtCoreVersion = 0;
	}

	bool retVal = true;

	iser::CArchiveTag lastConnectionTag("LastConnection", "LastConnection", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(lastConnectionTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_lastConnection);
	retVal = retVal && archive.EndTag(lastConnectionTag);

	iser::CArchiveTag computerNameTag("ComputerName", "Computer Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(computerNameTag);
	retVal = retVal && archive.Process(m_computerName);
	retVal = retVal && archive.EndTag(computerNameTag);

	iser::CArchiveTag servicesTag("Services", "Services", iser::CArchiveTag::TT_GROUP);
	retVal = retVal && archive.BeginTag(servicesTag);
	retVal = retVal && m_serviceCollection.Serialize(archive);
	retVal = retVal && archive.EndTag(servicesTag);

	iser::CArchiveTag versionTag("Version", "Version", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(versionTag);
	retVal = retVal && archive.Process(m_version);
	retVal = retVal && archive.EndTag(versionTag);

	if (imtCoreVersion > 9937){
		iser::CArchiveTag tracingLevelTag("TracingLevel", "TracingLevel", iser::CArchiveTag::TT_LEAF);
		retVal = retVal && archive.BeginTag(tracingLevelTag);
		retVal = retVal && archive.Process(m_tracingLevel);
		retVal = retVal && archive.EndTag(tracingLevelTag);
	}

	return retVal;
}


int CAgentInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CAgentInfo::CopyFrom(const IChangeable &object, CompatibilityMode /*mode*/)
{
	const CAgentInfo* sourcePtr = dynamic_cast<const CAgentInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_lastConnection = sourcePtr->m_lastConnection;
		m_computerName = sourcePtr->m_computerName;
		m_version = sourcePtr->m_version;
		m_tracingLevel = sourcePtr->m_tracingLevel;

		m_serviceCollection.ResetData();
		m_serviceCollection.CopyFrom(sourcePtr->m_serviceCollection);

		return true;
	}

	return false;
}


istd::IChangeableUniquePtr CAgentInfo::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CAgentInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CAgentInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_lastConnection = QDateTime();
	m_serviceCollection.ResetData();
	m_computerName.clear();
	m_tracingLevel = -1;

	return true;
}


} // namespace agentinodata


