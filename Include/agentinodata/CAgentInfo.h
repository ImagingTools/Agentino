#pragma once


// ImtCore includes
#include <agentinodata/IAgentInfo.h>
#include <imtbase/TIdentifiableWrap.h>


namespace agentinodata
{


class CAgentInfo: virtual public IAgentInfo
{
public:
	virtual void SetLastConnection(const QDateTime lastConnection);

	// reimplemented (agentinodata::IAgentInfo)
	virtual QString GetAgentName() const override;
	virtual void SetAgentName(const QByteArray& agentName) override;
	virtual QString GetAgentDescription() const override;
	virtual void SetAgentDescription(const QByteArray& agentDescription) override;
	virtual QByteArray GetHttpUrl() const override;
	virtual void SetHttpUrl(const QByteArray& httpUrl) override;
	virtual QByteArray GetWebSocketUrl() const override;
	virtual void SetWebSocketUrl(const QByteArray& webSocketUrl) override;
	virtual QDateTime GetLastConnection() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeable *CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QString m_name;
	QString m_description;
	QByteArray m_httpUrl;
	QByteArray m_webSocketUrl;
	QDateTime m_lastConnection;
};


typedef imtbase::TIdentifiableWrap<CAgentInfo> CIdentifiableAgentInfo;


} // namespace agentinodata



