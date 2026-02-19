// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QTimer>

// ACF includes
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/CObjectCollection.h>
#include <imtbase/TPluginManager.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IConnectionCollectionPlugin.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceController.h>


IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettings, imtservice::IConnectionCollectionPlugin);


namespace agentinodata
{


class CServiceControllerComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public IServiceController
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceController);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", true, "ServiceCollection");
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceController)
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override;
	virtual bool StartService(const QByteArray& serviceId) override;
	virtual bool StopService(const QByteArray& serviceId) override;

	// reimplemented (istd::ILogger)
	virtual void DecorateMessage(
				istd::IInformationProvider::InformationCategory category,
				int id,
				int flags,
				QString& message,
				QString& messageSource) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

public	Q_SLOTS:
	void OnReadyReadStandardError();
	void OnReadyReadStandardOutput();

private:
	QByteArray GetModuleName(QByteArray servicePath) const;
	void SetupProcess(QProcess& process, const QByteArray& programPath, const QStringList& arguments) const;
	void UpdateServiceVersion(const QByteArray& serviceId);
	void OnTimeout();
	void EmitChangeSignal(const QByteArray& serviceId, IServiceStatusInfo::ServiceStatus serviceStatus);

	struct ServiceProcess
	{
		IServiceStatusInfo::ServiceStatus lastStatus = IServiceStatusInfo::SS_UNDEFINED;
		int countOfStarting = 0;
	};

	class PluginManager: public imtbase::TPluginManager<
				imtservice::IConnectionCollectionPlugin,
				IMT_CREATE_PLUGIN_FUNCTION(ServiceSettings),
				IMT_DESTROY_PLUGIN_FUNCTION(ServiceSettings)>
	{
	public:
		PluginManager(
			const QByteArray& createMethodName,
			const QByteArray& destroyMethodName,
			imtbase::IPluginStatusMonitor* pluginStatusMonitorPtr)
		{
			m_createMethodName = createMethodName;
			m_destroyMethodName = destroyMethodName;
			m_pluginStatusMonitorPtr = pluginStatusMonitorPtr;
		}
	};

	typedef QMap<QByteArray, istd::TDelPtr<PluginManager>> PluginMap;
	mutable PluginMap m_pluginMap;

private:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);

	QMap<QByteArray, ServiceProcess> m_processMap;
	QTimer m_timer;
	QByteArray m_activeServiceId;
};


} // namespace agentinodata


