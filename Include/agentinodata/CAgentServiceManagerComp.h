// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QHash>

// ACF includes
#include <icomp/CComponentBase.h>
#include <ilog/TLoggerCompWrap.h>
#include <imod/TModelWrap.h>
#include <imod/CModelUpdateBridge.h>
#include <istd/TSingleFactory.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtbase/CObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


/**
	IServiceManager for the server (R1.3 / R1.4).

	Owns per-agent service collections **outside** CAgentInfo (no nested
	God-collection). This store is the server-side mirror that GQL CRUD,
	status/list, and topology read from; the agent stays the source of truth
	and the mirror is refreshed reactively from agent notifications.

	**The mirror is deliberately volatile — it is a cache, not a store.**
	It is not persisted: CAgentInfo drops any serialized nested services on load,
	so after a server restart the mirror starts empty even though agent *records*
	survive in the database. It is repopulated per agent by
	IServiceCollectionSynchronizer::SyncAgentServicesInMirror, which
	CAgentCollectionControllerComp triggers on every agent (re)connect.

	Consequence to keep in mind: between server start and an agent's reconnect,
	that agent's services are legitimately absent from list/topology responses.
	Never "repair" this by persisting the mirror — that would reintroduce a second
	source of truth that can silently diverge from the agent.

	On agent disconnect the services are intentionally *kept* (so topology still
	shows them) while CAgentChangeObserver marks their status UNDEFINED and their
	connection endpoints Offline.
*/
class CAgentServiceManagerComp:
			public ilog::CLoggerComponentBase,
			virtual public IServiceManager
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CAgentServiceManagerComp);
		I_REGISTER_INTERFACE(IServiceManager);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent records (identity only after R1.3)", true, "AgentCollection");
	I_END_COMPONENT;

	// reimplemented (IServiceManager)
	virtual bool AddService(
				const QByteArray& agentId,
				const IServiceInfo& serviceInfo,
				const QByteArray& serviceId = QByteArray(),
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString()) override;
	virtual bool RemoveServices(const QByteArray& agentId, const imtbase::ICollectionInfo::Ids& serviceIds) override;
	virtual bool SetService(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				const IServiceInfo& serviceInfo,
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString(),
				bool beQuiet = false) override;
	virtual bool ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const override;
	virtual IServiceInfo* GetService(const QByteArray& agentId, const QByteArray& serviceId) const override;

	/**
		Per-agent service collection (owned here, not nested in CAgentInfo).
		Creates an empty collection on first access if the agent exists.
	*/
	imtbase::IObjectCollection* GetServiceCollection(const QByteArray& agentId) const;

	/** Drop mirror for agent (unenroll / disconnect cleanup). */
	void EvictAgent(const QByteArray& agentId);

protected:
	virtual void OnComponentDestroyed() override;

private:
	imtbase::IObjectCollection* EnsureServiceCollection(const QByteArray& agentId) const;
	bool AgentExists(const QByteArray& agentId) const;

	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);

	struct Mirror
	{
		imod::TModelWrap<imtbase::CObjectCollection>* collection = nullptr;
	};

	mutable QHash<QByteArray, Mirror> m_mirrors;
};


} // namespace agentinodata
