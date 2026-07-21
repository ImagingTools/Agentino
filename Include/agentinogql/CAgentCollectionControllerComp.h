// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceManager.h>
#include <agentinogql/IEnrollmentController.h>
#include <agentinogql/IServiceCollectionSynchronizer.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents_fwd.h>


#undef GetObject


namespace agentinogql
{


class CAgentCollectionControllerComp: public QObject, public imtclientgql::TClientRequestManagerCompWrap<sdl::V1_0::agentino::CAgentCollectionControllerCompBase>
{
	Q_OBJECT
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<sdl::V1_0::agentino::CAgentCollectionControllerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CAgentCollectionControllerComp);
		I_ASSIGN(m_agentFactCompPtr, "AgentFactory", "Factory used for creation of the new agent instance", true, "AgentFactory");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection", false, "AgentStatusCollection");
		I_ASSIGN(m_serviceSynchronizerCompPtr, "ServiceSynchronizer", "Reconciles the mirrored service collection of a (re)connected agent", false, "ServiceSynchronizer");
		I_ASSIGN(m_enrollmentGateCompPtr, "EnrollmentGate", "Admits Approved agents; quarantines Pending; denies Rejected/Revoked", false, "EnrollmentStore");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Read access to enrollment status for the Agents list/editor", false, "EnrollmentStore");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections (R1.3)", false, "ServiceManager");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	
	// reimplemented (sdl::V1_0::agentino::CAgentCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::V1_0::agentino::CAgentsListGqlRequest& agentsListRequest,
				sdl::V1_0::agentino::CAgentItem& representationObject,
				QString& errorMessage) const override;
	virtual bool CreateRepresentationFromObject(
				const istd::IChangeable& data,
				const sdl::V1_0::agentino::CGetAgentGqlRequest& agentItemRequest,
				sdl::V1_0::agentino::CAgentData& representationPayload,
				QString& errorMessage) const override;
	virtual bool UpdateObjectFromRepresentationRequest(
				const ::imtgql::CGqlRequest& rawGqlRequest,
				const sdl::V1_0::agentino::CUpdateAgentGqlRequest& updateAgentRequest,
				istd::IChangeable& object,
				QString& errorMessage) const override;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual istd::IChangeableUniquePtr CreateObjectFromRequest(
				const imtgql::CGqlRequest& gqlRequest,
				QByteArray &objectId,
				QString& errorMessage) const override;
	virtual QJsonObject InsertObject(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

	// reimplemented (imtservergql::CObjectCollectionControllerCompBase)
	// Full reimplementation (not a wrap of BaseClass::ListObjects, matching this codebase's own
	// convention - e.g. CMessageCollectionControllerComp/CMirroredServiceCollectionControllerComp):
	// "status" is computed live per agent (ComputeAgentStatus), never stored, so the generic
	// ComplexFilter/filterModel machinery can't see it to filter on it - and for this in-memory
	// ObjectCollection, filterModel/offset/count are inert anyway (CObjectCollectionBase's
	// iterator ignores them). This filters by the "status" input directly and also returns a
	// full per-bucket count breakdown for the Agents page's filter buttons.
	virtual QJsonObject ListObjects(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	bool UpdateServiceStatusFromAgent(const QByteArray& agentId, const QByteArray& serviceId) const;
	void OnTimeout();
	GateDecision AdmitAgentFromRequest(const imtgql::CGqlRequest& gqlRequest, const QByteArray& agentId) const;
	bool IsAgentApproved(const QByteArray& agentId) const;
	// Connected/Disconnected/Undefined (Approved agent's live connection state) or
	// Pending/Suspended/Rejected/Revoked (enrollment state overrides connection state) - shared
	// by CreateRepresentationFromObject's "status" field and ListObjects' filter/counts.
	QString ComputeAgentStatus(const QByteArray& agentId) const;
	// Comparable value for one AgentItem field, for ListObjects' own sort (the in-memory
	// ObjectCollection this component sits on does not sort itself - see ListObjects). Covers
	// AgentHeadersDataProvider's SortableHeaderIds: name/description (String), computerName/
	// version (String), status (String, via ComputeAgentStatus), services (Int, count),
	// lastConnection (DateTime).
	QVariant GetAgentSortValue(const QByteArray& agentId, const QByteArray& fieldId) const;

protected:
	I_FACT(agentinodata::IAgentInfo, m_agentFactCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
	I_REF(IServiceCollectionSynchronizer, m_serviceSynchronizerCompPtr);
	I_REF(IEnrollmentGate, m_enrollmentGateCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);

	mutable QTimer m_timer;
	mutable QList<QByteArray> m_connectedAgents;
	// Agents that may fully sync (Approved). Pending agents are listed but not reconciled.
	mutable QSet<QByteArray> m_approvedAgents;
	bool m_timeoutRunning = false;
};


} // namespace agentinogql
