#pragma once


// ImtCore includes
#include <imtbase/PluginInterface.h>
#include <imtbase/IObjectCollection.h>
#include <imtgql/CGqlRequestHandlerCompBase.h>
#include <imtservice/IObjectCollectionPlugin.h>

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
	bool LoadPluginDirectory(const QString& pluginDirectoryPath, const QString& serviceName) const;

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);

	struct PluginInfo
	{
		PluginInfo()
			:pluginPtr(nullptr)
		{
		}

		imtservice::IObjectCollectionPlugin* pluginPtr;
		QString pluginPath;
		IMT_DESTROY_PLUGIN_FUNCTION(ServiceLog) destroyFunc;
	};

	typedef QMap<QString, PluginInfo> PluginMap;
	mutable PluginMap m_pluginMap;
};


} // namespace agentgql


