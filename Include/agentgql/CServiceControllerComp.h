// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services_fwd.h>


namespace agentgql
{


class CServiceControllerComp: public sdl::V1_0::agentino::CServicesGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CServicesGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerComp);
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", true, "ServiceController");
		I_ASSIGN(m_connectionCollectionProviderCompPtr, "ConnectionCollectionProvider", "Connection collection provider", true, "ConnectionCollectionProvider");
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection used to resolve service settings", false, "ServiceCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CServiceStatusResponse OnStartService(
				const sdl::V1_0::agentino::CStartServiceGqlRequest& startServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CServiceStatusResponse OnStopService(
				const sdl::V1_0::agentino::CStopServiceGqlRequest& stopServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::imtbase::CRemovedNotificationPayload OnServicesRemove(
				const sdl::V1_0::agentino::CServicesRemoveGqlRequest& removeServiceRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CServiceStatusResponse OnGetServiceStatus(
				const sdl::V1_0::agentino::CGetServiceStatusGqlRequest& getServiceStatusRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CUpdateConnectionUrlResponse OnUpdateConnectionUrl(
				const sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest& updateConnectionUrlRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CPluginInfo OnLoadPlugin(
				const sdl::V1_0::agentino::CLoadPluginGqlRequest& loadPluginRequest,
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


