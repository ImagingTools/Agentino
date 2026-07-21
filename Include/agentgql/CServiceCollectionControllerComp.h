// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
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
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services_fwd.h>


IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettings, imtservice::IConnectionCollectionPlugin);


#undef GetObject


namespace agentgql
{
 

class CServiceCollectionControllerComp:
			public sdl::V1_0::agentino::CServiceCollectionControllerCompBase,
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
	// reimplemented (sdl::V1_0::imtbase::CImtCollectionGqlHandlerCompBase)
	virtual sdl::V1_0::imtbase::CVisualStatus OnGetObjectVisualStatus(
		const sdl::V1_0::imtbase::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const override;
	
	// reimplemented (sdl::V1_0::agentino::CServiceCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::V1_0::agentino::CServicesListGqlRequest& servicesListRequest,
				sdl::V1_0::agentino::CServiceItem& representationObject,
				QString& errorMessage) const override;
	virtual bool CreateRepresentationFromObject(
				const istd::IChangeable& data,
				const sdl::V1_0::agentino::CGetServiceGqlRequest& getServiceRequest,
				sdl::V1_0::agentino::CServiceData& serviceData,
				QString& errorMessage) const override;
	virtual istd::IChangeableUniquePtr CreateObjectFromRepresentation(
				const sdl::V1_0::agentino::CServiceData& serviceDataRepresentation,
				QByteArray& newObjectId,
				QString& errorMessage) const override;
	virtual bool UpdateObjectFromRepresentationRequest(
				const ::imtgql::CGqlRequest& rawGqlRequest,
				const sdl::V1_0::agentino::CUpdateServiceGqlRequest& updateServiceRequest,
				istd::IChangeable& object,
				QString& errorMessage) const override;

	// reimplemented (imtservice::IConnectionCollectionProvider)
	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServicePath(const QString& servicePath) const override;
	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServiceId(const QByteArray& serviceId) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

private:
	/**
		True when the edited descriptor's wiring differs from what the running plugin
		instance currently uses: host/ports of any input connection or dependant link,
		or the tracing level. Only then is a stop → apply → start cycle worth it —
		renames, description or autostart edits must not restart a running service.
	*/
	bool IsConnectionUpdateRequired(
				agentinodata::IServiceInfo& serviceInfo,
				const imtservice::IConnectionCollection& connectionCollection) const;

	bool UpdateConnectionCollectionFromService(agentinodata::IServiceInfo& serviceInfo, imtservice::IConnectionCollection& connectionCollection) const;

	/**
		Seed the service descriptor's input/dependant connection collections from the
		connection list declared by the service's plugin. Existing entries are kept.
	*/
	void PopulateConnectionsFromPlugin(
				agentinodata::IServiceInfo& serviceInfo,
				const imtservice::IConnectionCollection& connectionCollection) const;

	// Checks whether another service in the collection already uses the given path.
	bool IsServicePathInUse(const QByteArray& servicePath, const QByteArray& excludeObjectId) const;
	
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


