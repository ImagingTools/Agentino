// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#include "agentinodata/CAgentStatusInfo.h"


// ACF includes
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>


namespace agentinodata
{


// public methods

CAgentStatusInfo::CAgentStatusInfo()
	:m_agentStatus(AS_UNDEFINED)
{
}


CAgentStatusInfo::CAgentStatusInfo(const QByteArray& agentId, AgentStatus serviceStatus)
	:m_agentId(agentId),
	m_agentStatus(serviceStatus)
{
}


void CAgentStatusInfo::SetAgentId(const QByteArray& agentId)
{
	if (m_agentId != agentId){
		istd::CChangeNotifier changeNotifier(this);

		m_agentId = agentId;
	}
}


void CAgentStatusInfo::SetAgentStatus(IAgentStatusInfo::AgentStatus status)
{
	if (m_agentStatus != status){
		istd::CChangeNotifier changeNotifier(this);

		m_agentStatus = status;
	}
}


// reimplemented (agentinodata::IAgentStatusInfo)

QByteArray CAgentStatusInfo::GetAgentId() const
{
	return m_agentId;
}


IAgentStatusInfo::AgentStatus CAgentStatusInfo::GetAgentStatus() const
{
	return m_agentStatus;
}


bool CAgentStatusInfo::Serialize(iser::IArchive &archive)
{
	bool retVal = true;

	iser::CArchiveTag serviceIdTag("AgentId", "Agent-ID", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceIdTag);
	retVal = retVal && archive.Process(m_agentId);
	retVal = retVal && archive.EndTag(serviceIdTag);

	iser::CArchiveTag statusTag("AgentStatus", "Agent status", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(statusTag);
	retVal = retVal && I_SERIALIZE_ENUM(AgentStatus, archive, m_agentStatus);
	retVal = retVal && archive.EndTag(statusTag);

	return retVal;
}


int CAgentStatusInfo::GetSupportedOperations() const
{
	return SO_COPY | SO_COMPARE | SO_RESET;
}


bool CAgentStatusInfo::CopyFrom(const IChangeable &object, CompatibilityMode /*mode*/)
{
	const CAgentStatusInfo* sourcePtr = dynamic_cast<const CAgentStatusInfo*>(&object);
	if (sourcePtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);

		m_agentId = sourcePtr->m_agentId;
		m_agentStatus = sourcePtr->m_agentStatus;

		return true;
	}

	return false;
}


istd::IChangeableUniquePtr CAgentStatusInfo::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CAgentStatusInfo);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CAgentStatusInfo::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);

	m_agentId.clear();
	m_agentStatus = AS_UNDEFINED;

	return true;
}


} // namespace agentinodata


