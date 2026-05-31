// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentgql
{


class CServiceControllerComp: public sdl::agentino::Services::CGraphQlHandlerCompBase
{
public:
	typedef CGraphQlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerComp);
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", true, "ServiceController");
		I_ASSIGN(m_connectionCollectionProviderCompPtr, "ConnectionCollectionProvider", "Connection collection provider", true, "ConnectionCollectionProvider");
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection used to resolve service settings", false, "ServiceCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::agentino::Services::CGraphQlHandlerCompBase)
	virtual sdl::agentino::Services::CServiceStatusResponse OnStartService(
				const sdl::agentino::Services::CStartServiceGqlRequest& startServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CServiceStatusResponse OnStopService(
				const sdl::agentino::Services::CStopServiceGqlRequest& stopServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::imtbase::ImtCollection::CRemovedNotificationPayload OnServicesRemove(
				const sdl::agentino::Services::CServicesRemoveGqlRequest& removeServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CServiceStatusResponse OnGetServiceStatus(
				const sdl::agentino::Services::CGetServiceStatusGqlRequest& getServiceStatusRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CUpdateConnectionUrlResponse OnUpdateConnectionUrl(
				const sdl::agentino::Services::CUpdateConnectionUrlGqlRequest& updateConnectionUrlRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CPluginInfo OnLoadPlugin(
				const sdl::agentino::Services::CLoadPluginGqlRequest& loadPluginRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CServiceSettingsPayload OnGetServiceSettings(
				const sdl::agentino::Services::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Services::CServiceSettingsPayload OnUpdateServiceSettings(
				const sdl::agentino::Services::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		Resolve the settings file path for a service and ensure it is located
		inside the service directory. Returns an empty path on failure.
	*/
	QString GetServiceSettingsFilePath(const QByteArray& serviceId, QString& errorMessage) const;

protected:
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
};


} // namespace agentgql


