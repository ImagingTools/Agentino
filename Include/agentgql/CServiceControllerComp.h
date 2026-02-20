// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
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
protected:
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);
};


} // namespace agentgql


