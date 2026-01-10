#pragma once


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
};


} // namespace agentgql


