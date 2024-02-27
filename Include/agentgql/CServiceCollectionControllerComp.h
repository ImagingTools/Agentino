#pragma once

// ImtCore includes
#include <imtgql/CObjectCollectionControllerCompBase.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IConnectionCollectionPlugin.h>

// ServiceManager includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceController.h>

IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettings, imtservice::IConnectionCollectionPlugin);


#undef GetObject


namespace agentgql
{


class CServiceCollectionControllerComp: public imtgql::CObjectCollectionControllerCompBase
{
public:
	typedef imtgql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionControllerComp);
		I_ASSIGN(m_serviceInfoFactCompPtr, "ServiceFactory", "Factory used for creation of the new service instance", true, "ServiceFactory");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", true, "ServiceController");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual istd::IChangeable* CreateObject(const QList<imtgql::CGqlObject>& inputParams, QByteArray &objectId, QString &name, QString &description, QString& errorMessage) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

	bool LoadPluginDirectory(const QString& pluginDirectoryPath, const QString& serviceName) const;

protected:
	I_FACT(agentinodata::IServiceInfo, m_serviceInfoFactCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);


	struct PluginInfo
	{
		PluginInfo()
			:pluginPtr(nullptr)
		{
		}

		imtservice::IConnectionCollectionPlugin* pluginPtr;
		QString pluginPath;
		IMT_DESTROY_PLUGIN_FUNCTION(ServiceSettings) destroyFunc;
	};

	typedef QMap<QString, PluginInfo> PluginMap;
	mutable PluginMap m_pluginMap;
};


} // namespace agentgql


