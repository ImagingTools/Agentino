// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Standard includes
#include <functional>

// Qt includes
#include <QtCore/QByteArray>

// Agentino includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <agentinodata/IServiceManager.h>


namespace agentinogql
{


/**
	Server-side read surface over the mirrored services of one agent.

	The agent owns its services; the server keeps a mirror of them per agent
	(IServiceManager) which is refreshed reactively from agent notifications. This
	controller answers the operator GUI's GraphQL *reads* against that mirror —
	the service list, the fields of one row, and the "Info" panel of one service.
	It never mutates anything and never talks to an agent: commands go through
	CServiceControllerProxyComp instead.

	Which agent is addressed comes from the "clientid" request header.

	Note that the inherited ObjectCollection means something different here than in
	the agent-side base class: on the agent it holds the services themselves, on the
	server it holds the *agent records*, and the services are reached through
	ServiceManager.
*/
class CMirroredServiceCollectionControllerComp: public agentgql::CServiceCollectionControllerComp
{
public:
	typedef agentgql::CServiceCollectionControllerComp BaseClass;

	I_BEGIN_COMPONENT(CMirroredServiceCollectionControllerComp);
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", true, "ServiceStatusCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent record collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent mirrored service collections", false, "ServiceManager");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::imtbase::CImtCollectionGqlHandlerCompBase)
	virtual sdl::V1_0::imtbase::CGetElementMetaInfoPayload OnGetElementMetaInfo(
				const sdl::V1_0::imtbase::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

	// reimplemented (agentgql::CServiceCollectionControllerComp) — the inherited version reads
	// m_objectCollectionCompPtr, which here holds agent records, not services (see class doc),
	// so it can never find the requested service. Look it up in the mirror instead.
	virtual sdl::V1_0::imtbase::CVisualStatus OnGetObjectVisualStatus(
				const sdl::V1_0::imtbase::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				QJsonObject& itemObj,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual QJsonObject ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	/**
		Mirrored service collection of \a agentId, or nullptr when that agent has
		nothing mirrored yet (it has not connected/reconciled since server start).
	*/
	imtbase::IObjectCollection* GetMirroredServices(const QByteArray& agentId) const;

	/**
		Look up one mirrored service across **all** agents.

		Only the server can do this — an agent knows nothing about its peers — and it
		is what makes cross-agent connections resolvable. Calls \a visitor for every
		mirrored service until it returns \c true.
		\param visitor	(agentId, agentName, serviceInfo) -> stop iterating?
	*/
	void ForEachMirroredService(
				const std::function<bool(
							const QByteArray& agentId,
							const QString& agentName,
							const QByteArray& serviceId,
							agentinodata::IServiceInfo& serviceInfo)>& visitor) const;

	/**
		Human-readable "serviceType@agent (http: p; ws: p)" entries for every service
		that consumes the endpoint \a endpointId (a ServiceEndpointId — consumer links
		store the full id, never the bare connection id).
	*/
	QStringList GetConsumersOfConnection(const QByteArray& endpointId) const;

	/**
		The published connection addressed by \a endpointId (a ServiceEndpointId):
		either an input-connection element of the producing service or the uuid of one
		of its incoming connections. Invalid pointer when the producer is not mirrored.
	*/
	istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> FindInputConnection(
				const QByteArray& endpointId) const;

	/**
		Why \a serviceId cannot run: the services it depends on that are not running.
	*/
	QStringList GetDependantStatusInfo(const QByteArray& serviceId) const;

	/** Translate \a sourceText in the language of \a gqlRequest. */
	QString Translate(const imtgql::CGqlRequest& gqlRequest, const char* sourceText) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
};


} // namespace agentinogql
