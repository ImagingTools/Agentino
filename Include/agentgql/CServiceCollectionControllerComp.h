#pragma once


// ImtCore includes
#include <imtservergql/CObjectCollectionControllerCompBase.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IConnectionCollectionPlugin.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtbase/TPluginManager.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceController.h>

IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettings, imtservice::IConnectionCollectionPlugin);


#undef GetObject


namespace agentgql
{


class CServiceCollectionControllerComp:
			public imtservergql::CObjectCollectionControllerCompBase,
			virtual public imtservice::IConnectionCollectionProvider
{
public:
	typedef imtservergql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionControllerComp);
		I_REGISTER_INTERFACE(imtservice::IConnectionCollectionProvider)
		I_ASSIGN(m_serviceInfoFactCompPtr, "ServiceFactory", "Factory used for creation of the new service instance", false, "ServiceFactory");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", false, "ServiceController");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::imtbase::ImtCollection::CGraphQlHandlerCompBase)
	virtual sdl::imtbase::ImtCollection::CVisualStatus OnGetObjectVisualStatus(
		const sdl::imtbase::ImtCollection::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const override;

	// reimplemented (imtservergql::CObjectCollectionControllerCompBase)
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
	virtual istd::IChangeable* CreateObjectFromRequest(const imtgql::CGqlRequest& gqlRequest, QByteArray &objectId, QString& errorMessage) const override;

	// reimplemented (imtservice::IConnectionCollectionProvider)
	virtual std::shared_ptr<imtservice::IConnectionCollection> GetConnectionCollection(const QByteArray& serviceId) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

	virtual imtbase::IObjectCollection* GetObjectCollection(const QByteArray& id = QByteArray()) const;

protected:
	I_FACT(agentinodata::IServiceInfo, m_serviceInfoFactCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);

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
};


} // namespace agentgql


