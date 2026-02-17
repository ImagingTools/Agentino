// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
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
	virtual void SetServiceName(const QString& name);
	virtual void SetServiceDescription(const QString& description);
	virtual void SetServicePath(const QByteArray& servicePath);
	virtual void SetServiceSettingsPath(const QByteArray& serviceSettingsPath);
	virtual void SetStartScriptPath(const QByteArray& startScriptPath);
	virtual void SetStopScriptPath(const QByteArray& stopScriptPath);
	virtual void SetServiceArguments(const QByteArrayList& serviceArguments);
	virtual void SetIsAutoStart(bool isAutoStart);
	virtual void SetServiceTypeId(const QByteArray& serviceTypeName);
	virtual void SetServiceVersion(const QString& serviceVersion);

	// reimplemented (agentinodata::IServiceInfo)
	virtual QString GetServiceName() const override;
	virtual QString GetServiceDescription() const override;
	virtual SettingsType GetSettingsType() const override;
	virtual QString GetServiceVersion() const override;
	virtual QString GetServiceTypeId() const override;
	virtual QByteArray GetServicePath() const override;
	virtual QByteArray GetServiceSettingsPath() const override;
	virtual QByteArrayList GetServiceArguments() const override;
	virtual bool IsAutoStart() const override;
	virtual QByteArray GetStartScriptPath() const override;
	virtual QByteArray GetStopScriptPath() const override;
	virtual imtbase::IObjectCollection* GetInputConnections() override;
	virtual imtbase::IObjectCollection* GetDependantServiceConnections() override;

	// reimplemented (ilog::ITracingConfiguration)
	virtual int GetTracingLevel() const override;
	virtual void SetTracingLevel(int tracingLevel) override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	SettingsType m_settingsType;
	QString m_serviceVersion;
	QString m_serviceName;
	QString m_serviceDescription;
	QString m_serviceTypeId;
	QByteArray m_path;
	QByteArray m_settingsPath;
	QByteArray m_startScriptPath;
	QByteArray m_stopScriptPath;
	QByteArrayList m_arguments;
	int m_tracingLevel;

	imtbase::CObjectCollection m_inputConnections;
	imtbase::CObjectCollection m_dependantServiceConnections;
	bool m_isAutoStart;
};


typedef imtbase::TIdentifiableWrap<CServiceInfo> CIdentifiableServiceInfo;


} // namespace agentinodata



