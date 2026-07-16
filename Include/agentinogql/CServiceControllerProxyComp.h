// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QSet>

// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinogql/IServiceCollectionSynchronizer.h>
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>

#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes_fwd.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services_fwd.h>


namespace imtgql
{
class CGqlRequest;
}


namespace agentinogql
{


class CServiceControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
										sdl::V1_0::agentino::CServicesGqlHandlerCompBase>,
			virtual public IServiceCollectionSynchronizer
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
		sdl::V1_0::agentino::CServicesGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerProxyComp);
		I_REGISTER_INTERFACE(IServiceCollectionSynchronizer);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "ServceManager", true, "ServiceManager");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", false, "AgentCollection");
	I_END_COMPONENT;

	// reimplemented (IServiceCollectionSynchronizer)
	virtual bool SyncServiceInMirror(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				QString& errorMessage) override;
	virtual bool RemoveServicesInMirror(
				const QByteArray& agentId,
				const QByteArrayList& serviceIds,
				QString& errorMessage) override;
	virtual bool SyncAgentServicesInMirror(
				const QByteArray& agentId,
				QString& errorMessage) override;

protected:
	virtual sdl::V1_0::agentino::CServiceData OnGetService(
		const sdl::V1_0::agentino::CGetServiceGqlRequest& getServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	virtual sdl::V1_0::imtbase::CUpdatedNotificationPayload OnUpdateService(
		const sdl::V1_0::agentino::CUpdateServiceGqlRequest& updateServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	virtual sdl::V1_0::imtbase::CAddedNotificationPayload OnAddService(
		const sdl::V1_0::agentino::CAddServiceGqlRequest& addServiceRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;
	
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
	virtual sdl::V1_0::agentino::CServiceSettingsPayload OnGetServiceSettings(
		const sdl::V1_0::agentino::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CServiceSettingsPayload OnUpdateServiceSettings(
		const sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const override;
	// Handles the generic 'RemoveElements' command (imtbase collection schema) for the
	// 'Services' collection - forwards it to the owning Agent, mirroring OnServicesRemove.
	virtual sdl::V1_0::imtbase::CRemoveElementsPayload OnRemoveElements(
		const sdl::V1_0::imtbase::CRemoveElementsGqlRequest& removeElementsRequest,
		const ::imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const;

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual QJsonObject CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;

	// Live agent → server collection notify (does not use reverse subscription path).
	QJsonObject HandleAgentServiceCollectionNotify(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;
	
private:
	template<class SdlGqlRequest, class SdlResponse>
	QJsonObject CreateResponse(
		const imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage,
		std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const;
	bool SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status) const;
	bool SetServiceStatus(const QByteArray& serviceId, sdl::V1_0::agentino::ServiceStatus status) const;
	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CDependantConnectionInfo>> GetConnectionsModel(const QByteArray& connectionUsageId) const;
	bool GetConnectionObjectData(
		const imtbase::IObjectCollection::Id& connectionId,
		imtbase::IObjectCollection::DataPtr& connectionDataPtr) const;
	void UpdateUrlFromDependantConnection(sdl::V1_0::agentino::CServiceData& serviceData) const;
	istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> GetDependantServerConnectionParam(const QByteArray& dependantId) const;

	sdl::V1_0::imtbase::CServerConnectionParam GetServerConnectionParam(
		const sdl::V1_0::agentino::CServiceData& serviceData,
		const QByteArray& connectionId) const;
	
	QByteArrayList GetChangedConnectionUrl(
		const sdl::V1_0::agentino::CServiceData& serviceData1,
		const sdl::V1_0::agentino::CServiceData& serviceData2) const;
	QByteArrayList GetServiceIdsByDependantId(const QByteArray& dependantId) const;
	bool UpdateConnectionForService(
		const QByteArray& serviceId,
		const QByteArray& agentId,
		const QByteArray& connectionId,
		const sdl::V1_0::imtbase::CServerConnectionParam& connectionParam) const;

	QByteArrayList GetMirrorServiceIds(const QByteArray& agentId) const;
	void RemoveServiceStatuses(const QByteArrayList& serviceIds) const;
	/**
		Drop mirror entries for the same agent that share \a servicePath but not \a keepServiceId.
		Needed when a service is recreated with a new UUID after a missed/failed live remove:
		topology would otherwise show the stale entry (status UNDEFINED) next to the new one.
	*/
	void RemoveStaleMirrorServicesByPath(
				const QByteArray& agentId,
				const QByteArray& keepServiceId,
				const QByteArray& servicePath) const;
	static void AppendServicesListFields(imtgql::CGqlRequest& gqlRequest);
	static void AppendServiceDataFields(imtgql::CGqlRequest& gqlRequest);
	
protected:
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);

	// Agents currently inside SyncAgentServicesInMirror (re-entrancy via SendModelRequest event loop).
	QSet<QByteArray> m_agentsBeingReconciled;
};


} // namespace agentinogql


