#pragma once


// ImtCore includes
#include <imtbase/TIdentifiableWrap.h>
#include <imtbase/CObjectCollection.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>


namespace agentinodata
{


class CAgentInfo: virtual public IAgentInfo
{
public:
	CAgentInfo();

	virtual void SetLastConnection(const QDateTime& lastConnection);
	virtual void SetComputerName(const QString& computerName);

	// reimplemented (agentinodata::IAgentInfo)
	virtual QDateTime GetLastConnection() const override;
	virtual QString GetComputerName() const override;
	virtual imtbase::IObjectCollection* GetServiceCollection() override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeable* CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QDateTime m_lastConnection;
	QString m_computerName;
	imtbase::CObjectCollection m_serviceCollection;
};


typedef imtbase::TIdentifiableWrap<CAgentInfo> CIdentifiableAgentInfo;


} // namespace agentinodata



