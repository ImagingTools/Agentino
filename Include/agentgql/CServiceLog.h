// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// imtbase/TPluginManager.h includes <windows.h>: keep it lean, otherwise DDEML macros
// (e.g. ST_CONNECTED) leak into the package registry and break other headers there.
#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#	define WIN32_LEAN_AND_MEAN
#endif

// Qt includes
#include <QtCore/QMutex>

// ImtCore includes
#include <imtbase/PluginInterface.h>
#include <imtbase/TPluginManager.h>
#include <imtservice/IObjectCollectionPlugin.h>


IMT_DECLARE_PLUGIN_INTERFACE(ServiceLog, imtservice::IObjectCollectionPlugin);


namespace agentgql
{


class CServiceLog
{
protected:
	class PluginManager: public imtbase::TPluginManager<
				imtservice::IObjectCollectionPlugin,
				IMT_CREATE_PLUGIN_FUNCTION(ServiceLog),
				IMT_DESTROY_PLUGIN_FUNCTION(ServiceLog)>
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

	// Guards 'm_pluginMap': GraphQL requests are dispatched across a worker-thread pool
	// (see imtrest::CWorkerManagerComp, default ThreadsLimit=5), so concurrent 'GetServiceLog'
	// requests can call into the same component instance from different threads at once.
	mutable QMutex m_pluginMapMutex;
};


} // namespace agentgql


