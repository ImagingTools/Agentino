#pragma once


// ImtCore includes
#include "istd/TInterfacePtr.h"
#include <imtservergql/CObjectCollectionControllerCompBase.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IConnectionCollectionPlugin.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtbase/TPluginManager.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettings, imtservice::IConnectionCollectionPlugin);


#undef GetObject


namespace agentgql
{
 

class CServiceCollectionControllerComp:
			public sdl::agentino::Services::CServiceCollectionControllerCompBase,
			virtual public imtservice::IConnectionCollectionProvider
{
public:
	typedef CServiceCollectionControllerCompBase BaseClass;

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
	
	// reimplemented (sdl::agentino::Services::CServiceCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::agentino::Services::CServicesListGqlRequest& servicesListRequest,
				sdl::agentino::Services::CServiceItem::V1_0& representationObject,
				QString& errorMessage) const override;
	virtual bool CreateRepresentationFromObject(
				const istd::IChangeable& data,
				const sdl::agentino::Services::CGetServiceGqlRequest& getServiceRequest,
				sdl::agentino::Services::CServiceData::V1_0& serviceData,
				QString& errorMessage) const override;
	virtual istd::IChangeableUniquePtr CreateObjectFromRepresentation(
				const sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation,
				QByteArray& newObjectId,
				QString& errorMessage) const override;
	virtual bool UpdateObjectFromRepresentationRequest(
				const ::imtgql::CGqlRequest& rawGqlRequest,
				const sdl::agentino::Services::CUpdateServiceGqlRequest& updateServiceRequest,
				istd::IChangeable& object,
				QString& errorMessage) const override;

	// reimplemented (imtservice::IConnectionCollectionProvider)
	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServicePath(const QString& servicePath) const override;
	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServiceId(const QByteArray& serviceId) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

private:
	bool CheckInputPortsUpdated(agentinodata::IServiceInfo& serviceInfo, const imtservice::IConnectionCollection& connectionCollection) const;
	bool UpdateConnectionCollectionFromService(agentinodata::IServiceInfo& serviceInfo, imtservice::IConnectionCollection& connectionCollection) const;
	
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

	struct PluginInfo
	{
		istd::TDelPtr<PluginManager> pluginManagerPtr;
		imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = nullptr;
	};

	typedef QMap<QByteArray, PluginInfo> PluginMap;
	mutable PluginMap m_pluginMap;
};


} // namespace agentgql


