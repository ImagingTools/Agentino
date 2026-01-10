#pragma once


// Agentino includes
#include <agentinodata/IAgentStatusInfo.h>


namespace agentinodata
{


class CAgentStatusInfo: virtual public IAgentStatusInfo
{
public:
	CAgentStatusInfo();
	CAgentStatusInfo(const QByteArray& agentId, AgentStatus serviceStatus);

	virtual void SetAgentId(const QByteArray& agentId);
	virtual void SetAgentStatus(AgentStatus status);

	// reimplemented (agentinodata::IAgentStatusInfo)
	virtual QByteArray GetAgentId() const override;
	virtual AgentStatus GetAgentStatus() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QByteArray m_agentId;
	AgentStatus m_agentStatus;
};


} // namespace agentinodata


