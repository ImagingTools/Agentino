#pragma once


// ImtCore includes
#include <imtbase/TIdentifiableWrap.h>
#include <imtbase/CObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


class CServiceInfo: virtual public IServiceInfo
{

public:
	CServiceInfo(const QString& typeName = QString(), SettingsType settingsType = ST_PLUGIN);
	virtual void SetServicePath(const QByteArray& servicePath);
	virtual void SetServiceSettingsPath(const QByteArray& serviceSettingsPath);
	virtual void SetServiceArguments(const QByteArrayList& serviceArguments);
	virtual void SetIsAutoStart(bool isAutoStart);
	virtual void SetServiceTypeName(const QByteArray& serviceTypeName);

	// reimplemented (agentinodata::IServiceInfo)
	virtual SettingsType GetSettingsType() const override;
	virtual QString GetServiceTypeName() const override;
	virtual QByteArray GetServicePath() const override;
	virtual QByteArray GetServiceSettingsPath() const override;
	virtual QByteArrayList GetServiceArguments() const override;
	virtual bool IsAutoStart() const override;
	virtual imtbase::IObjectCollection* GetInputConnections() override;
	virtual imtbase::IObjectCollection* GetDependantServiceConnections() override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeable *CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	SettingsType m_settingsType;
	QString m_serviceTypeName;
	QByteArray m_path;
	QByteArray m_settingsPath;
	QByteArrayList m_arguments;

	imtbase::CObjectCollection m_inputConnections;
	imtbase::CObjectCollection m_dependantServiceConnections;
	bool m_isAutoStart;
};


typedef imtbase::TIdentifiableWrap<CServiceInfo> CIdentifiableServiceInfo;


} // namespace agentinodata



