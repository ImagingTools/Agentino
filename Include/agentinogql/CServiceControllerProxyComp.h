// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QTimer>

// ImtCore includes
#include <imtclientgql/TAsyncClientRequestManagerCompWrap.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinogql/IEnrollmentController.h>
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


/**
	Server proxy for agent services.

	Inherits stacked request managers (separate classes, not dual-logic one wrap):
	- \c TClientRequestManagerCompWrap — sync \c ApiClient (\c IGqlClient)
	- \c TAsyncClientRequestManagerCompWrap — async \c AsyncApiClient (\c IAsyncGqlClient)

	Mirror reconcile / Start / Stop use async only (no Wait on the WS path).
	Other agent mutations still use sync \c SendModelRequest via \c ApiClient.
*/
class CServiceControllerProxyComp:
			public QObject,
			public imtclientgql::TAsyncClientRequestManagerCompWrap<
						imtclientgql::TClientRequestManagerCompWrap<
									sdl::V1_0::agentino::CServicesGqlHandlerCompBase>>,
			virtual public IServiceCollectionSynchronizer
{
	Q_OBJECT
public:
	typedef imtclientgql::TAsyncClientRequestManagerCompWrap<
				imtclientgql::TClientRequestManagerCompWrap<
							sdl::V1_0::agentino::CServicesGqlHandlerCompBase>> BaseClass;

	CServiceControllerProxyComp();

	I_BEGIN_COMPONENT(CServiceControllerProxyComp);
		I_REGISTER_INTERFACE(IServiceCollectionSynchronizer);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "ServceManager", true, "ServiceManager");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", false, "AgentCollection");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Gate reconcile/mirror for non-Approved agents", false, "EnrollmentStore");
		// ApiClient + AsyncApiClient come from the stacked base wraps.
		// Wire both to WebSocketServerFramework (IGqlClient via SyncAdapter, IAsyncGqlClient via SubscriptionManager).
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
	// Resolves dependantConnectionId across the whole fleet (an agent cannot see its
	// peers), then forwards to the owning agent with the resolved param attached.
	virtual sdl::V1_0::agentino::CSetOutputConnectionResponse OnSetOutputConnection(
		const sdl::V1_0::agentino::CSetOutputConnectionGqlRequest& setOutputConnectionRequest,
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
	// Candidate producers across the whole fleet, answered from the mirror without
	// forwarding to any agent (the pick-list is inherently cross-agent).
	virtual sdl::V1_0::agentino::CAvailableConnectionsPayload OnAvailableConnections(
		const sdl::V1_0::agentino::CAvailableConnectionsGqlRequest& availableConnectionsRequest,
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
	/** Candidate producers across the fleet for one output slot (used by OnAvailableConnections). */
	QList<sdl::V1_0::agentino::CDependantConnectionInfo> BuildAvailableConnections(const QByteArray& connectionUsageId) const;
	istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> GetDependantServerConnectionParam(const QByteArray& dependantId) const;

	/**
		One producer address (main or extern) whose URL changed, with its new value.
		Plain fields, not the SDL CServerConnectionParam type - this header only has the
		SDL types forward-declared, and this struct (unlike the rest of the class) needs
		to hold one by value.
	*/
	struct ChangedConnectionUrl
	{
		// The producer-local id this address is reached by: an input connection's own id
		// for the main address, or one of its extern entries' uuid for an extern address -
		// either way, the second half of the ServiceEndpointId consumers link against.
		QByteArray addressableId;
		QString host;
		bool isSecure = false;
		int httpPort = -1;
		QString httpPath;
		int wsPort = -1;
	};

	/**
		Every main or extern producer address in \a serviceData2 whose host/port differs
		from \a serviceData1 - covers extern connections too, since a consumer can be
		linked to one specific extern address independently of the main one (see
		AppendAvailableConnectionsFromServiceCollection).
	*/
	QList<ChangedConnectionUrl> GetChangedConnectionUrl(
		const sdl::V1_0::agentino::CServiceData& serviceData1,
		const sdl::V1_0::agentino::CServiceData& serviceData2) const;

	/** One service that consumes an endpoint, together with the agent hosting it. */
	struct ConsumerRef
	{
		QByteArray agentId;
		QByteArray serviceId;
		// The consumer's own dependant-link element id. The agent-side
		// UpdateConnectionUrl handler writes the new URL into the consumer plugin's
		// connection collection under THIS id — never under the producer's endpoint id.
		QByteArray slot;
	};

	/**
		Every mirrored service whose dependant connection points at \a endpointId
		(a ServiceEndpointId). Used to push a changed producer URL to its consumers,
		each on its own agent.
	*/
	QVector<ConsumerRef> FindConsumersOfEndpoint(const QByteArray& endpointId) const;

	/**
		The mirrored service with \a serviceId, wherever in the fleet it lives, or
		nullptr when no agent mirrors it.
	*/
	agentinodata::IServiceInfo* FindMirroredService(const QByteArray& serviceId) const;

	bool UpdateConnectionForService(
		const QByteArray& serviceId,
		const QByteArray& agentId,
		const QByteArray& connectionId,
		const sdl::V1_0::imtbase::CServerConnectionParam& connectionParam) const;

	QByteArrayList GetMirrorServiceIds(const QByteArray& agentId) const;
	void RemoveServiceStatuses(const QByteArrayList& serviceIds) const;
	static void AppendServicesListFields(imtgql::CGqlRequest& gqlRequest);
	static void AppendServiceDataFields(imtgql::CGqlRequest& gqlRequest);
	
protected:
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);

	// Agents with an async ServicesList reconcile in flight. Coalesces overlapping
	// reconciles: a request that arrives while one is running is deferred, not dropped.
	// (Per-service GetService inside ApplyServicesListReconcile is async + list-seeded, so a
	// notify can re-enter during that pass — this guard keeps the passes serialized.)
	QSet<QByteArray> m_agentsBeingReconciled;
	// Agents that need another reconcile after the in-flight one completes (queued).
	// Touched only on this component's thread (via QueuedConnection).
	mutable QSet<QByteArray> m_pendingReconcile;

	void QueueReconcile(const QByteArray& agentId) const;
	/**
		Queue a non-blocking GetService+mirror after the notify response is sent.
		Must not run inside the notify handler (agent blocked in Wait on notify).
		Must not block the server thread with sync Wait — that freezes the UI and
		floods the agent with concurrent /Agent/graphql work.
	*/
	void QueueServiceSync(const QByteArray& agentId, const QByteArray& serviceId) const;
	/** Apply an already-fetched ServiceData into the server mirror (insert or update). */
	bool ApplyServiceDataToMirror(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				const sdl::V1_0::agentino::CServiceData& serviceData,
				QString& errorMessage);
	/**
		Non-blocking GetService to the agent, then ApplyServiceDataToMirror.
		Preferred for live notify path. Returns false only if dispatch failed.
	*/
	bool StartAsyncServiceSync(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				QString& errorMessage);
	/**
		Apply a completed ServicesList payload: diff against the mirror, drop stale
		services, and (re)sync the current ones.
	*/
	bool ApplyServicesListReconcile(
				const QByteArray& agentId,
				const sdl::V1_0::agentino::CServiceListPayload& listPayload,
				QString& errorMessage);
	/** Non-blocking ServicesList, then ApplyServicesListReconcile on completion. */
	bool StartAsyncAgentReconcile(const QByteArray& agentId, QString& errorMessage);

private Q_SLOTS:
	void OnDeferredReconcile();
};


} // namespace agentinogql


