// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
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
protected:
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);
};


} // namespace agentgql


