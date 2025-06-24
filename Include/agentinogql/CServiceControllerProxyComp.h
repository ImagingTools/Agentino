#pragma once


// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <generatedfiles/imtbasesdl/sdl/1.0/cpp/imtbasetypes.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinogql
{


class CServiceControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
										sdl::agentino::Services::CGraphQlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
		sdl::agentino::Services::CGraphQlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerProxyComp);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "ServceManager", true, "ServiceManager");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", false, "AgentCollection");
	I_END_COMPONENT;

protected:
	virtual sdl::agentino::Services::CServiceData OnGetService(
		const sdl::agentino::Services::CGetServiceGqlRequest& getServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	virtual sdl::imtbase::ImtCollection::CUpdatedNotificationPayload OnUpdateService(
		const sdl::agentino::Services::CUpdateServiceGqlRequest& updateServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	virtual sdl::imtbase::ImtCollection::CAddedNotificationPayload OnAddService(
		const sdl::agentino::Services::CAddServiceGqlRequest& addServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	
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
	
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	
private:
	template<class SdlGqlRequest, class SdlResponse>
	imtbase::CTreeItemModel* CreateResponse(
		const imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage,
		std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const;
	bool SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status) const;
	bool SetServiceStatus(const QByteArray& serviceId, sdl::agentino::Services::ServiceStatus status) const;
	QList<sdl::agentino::Services::CElement::V1_0> GetConnectionsModel(const QByteArray& connectionUsageId) const;
	bool GetConnectionObjectData(
		const imtbase::IObjectCollection::Id& connectionId,
		imtbase::IObjectCollection::DataPtr& connectionDataPtr) const;
	void UpdateUrlFromDependantConnection(sdl::agentino::Services::CServiceData::V1_0& serviceData) const;
	std::shared_ptr<const imtcom::CServerConnectionInterfaceParam> GetDependantServerConnectionParam(const QByteArray& dependantId) const;

	sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 GetServerConnectionParam(
		const sdl::agentino::Services::CServiceData::V1_0& serviceData,
		const QByteArray& connectionId) const;
	
	QByteArrayList GetChangedConnectionUrl(
		const sdl::agentino::Services::CServiceData::V1_0& serviceData1,
		const sdl::agentino::Services::CServiceData::V1_0& serviceData2) const;
	QByteArrayList GetServiceIdsByDependantId(const QByteArray& dependantId) const;
	bool UpdateConnectionForService(
		const QByteArray& serviceId,
		const QByteArray& agentId,
		const QByteArray& connectionId,
		const sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& connectionParam) const;
	
protected:
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentinogql


