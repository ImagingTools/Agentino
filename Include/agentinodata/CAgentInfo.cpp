#include <agentinodata/CAgentInfo.h>


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

void CAgentInfo::SetLastConnection(const QDateTime lastConnection)
{
	m_lastConnection = lastConnection;
}


// reimplemented (agentinodata::IAgentInfo)
QString CAgentInfo::GetAgentName() const
{
	return m_name;
}


void CAgentInfo::SetAgentName(const QByteArray& agentName)
{
	m_name = agentName;
}


QString CAgentInfo::GetAgentDescription() const
{
	return m_description;
}


void CAgentInfo::SetAgentDescription(const QByteArray& agentDescription)
{
	m_description = agentDescription;
}


QByteArray CAgentInfo::GetHttpUrl() const
{
	return m_httpUrl;
}


void CAgentInfo::SetHttpUrl(const QByteArray& httpUrl)
{
	m_httpUrl = httpUrl;
}


QByteArray CAgentInfo::GetWebSocketUrl() const
{
	return m_webSocketUrl;
}


void CAgentInfo::SetWebSocketUrl(const QByteArray& webSocketUrl)
{
	m_webSocketUrl = webSocketUrl;
}


QDateTime CAgentInfo::GetLastConnection() const
{
	return m_lastConnection;
}


// reimplemented (iser::ISerializable)
bool CAgentInfo::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = true;

	iser::CArchiveTag nameTag("Name", "Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(nameTag);
	retVal = retVal && archive.Process(m_name);
	retVal = retVal && archive.EndTag(nameTag);

	iser::CArchiveTag descriptionTag("Description", "Description", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(descriptionTag);
	retVal = retVal && archive.Process(m_description);
	retVal = retVal && archive.EndTag(descriptionTag);

	iser::CArchiveTag httpUrlTag("HttpUrl", "HttpUrl", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(httpUrlTag);
	retVal = retVal && archive.Process(m_httpUrl);
	retVal = retVal && archive.EndTag(httpUrlTag);

	iser::CArchiveTag webSocketUrlTag("WebSocketUrl", "WebSocketUrl", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(webSocketUrlTag);
	retVal = retVal && archive.Process(m_webSocketUrl);
	retVal = retVal && archive.EndTag(webSocketUrlTag);

	iser::CArchiveTag lastConnectionTag("LastConnection", "LastConnection", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(lastConnectionTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeDateTime(archive, m_lastConnection);
	retVal = retVal && archive.EndTag(lastConnectionTag);

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

		m_name = sourcePtr->m_name;
		m_description = sourcePtr->m_description;
		m_httpUrl = sourcePtr->m_httpUrl;
		m_webSocketUrl = sourcePtr->m_webSocketUrl;
		m_lastConnection = sourcePtr->m_lastConnection;

		return true;
	}

	return false;
}


istd::IChangeable *CAgentInfo::CloneMe(CompatibilityMode mode) const
{
	istd::TDelPtr<CAgentInfo> clonePtr(new CAgentInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr.PopPtr();
	}

	return nullptr;
}


bool CAgentInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_name.clear();
	m_description.clear();
	m_name.clear();
	m_httpUrl.clear();
	m_webSocketUrl.clear();
	m_lastConnection = QDateTime();

	return true;
}


} // namespace agentinodata


