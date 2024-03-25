#pragma once


// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtbase/PluginInterface.h>
#include <imtbase/IObjectCollection.h>
#include <imtgql/CGqlRequestHandlerCompBase.h>
#include <imtservice/IObjectCollectionPlugin.h>
#include <imtbase/TPluginManager.h>

IMT_DECLARE_PLUGIN_INTERFACE(ServiceLog, imtservice::IObjectCollectionPlugin);


namespace agentgql
{


class CServiceLogControllerComp: public imtgql::CGqlRequestHandlerCompBase
{
public:
	typedef imtgql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceLogControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", false, "ServiceCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);

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


