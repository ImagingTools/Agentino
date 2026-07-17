// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlPublisherCompBase.h>

// Agentino includes
#include <agentinodata/IServiceController.h>


namespace agentinogql
{


/**
	Observes a status model (typically ServiceController or SubscriptionController) and
	publishes status payloads on configured CommandIds.

	Roles by deployment:
	- Agent: Model=ServiceController, ServiceStatusCollection unset.
	  Source of truth is the live process controller; this component only publishes
	  OnServiceStatusChanged (local GUI) / OnAgentServiceStatusChanged (central server).
	- Server: Model=AgentSubscriptionController (CN_STATUS_CHANGED from agent push) and/or
	  ServiceStatusCollection wired so the optional mirror is kept in sync.

	ServiceStatusCollection is always optional. Missing collection must never block PublishData.
*/
class CServiceSubscriberControllerComp:
			public imtservergql::CGqlPublisherCompBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceSubscriberControllerComp);
		I_ASSIGN(m_modelCompPtr, "Model", "Status model that emits CN_STATUS_CHANGED (ServiceController or SubscriptionController)", true, "Model");
		I_ASSIGN(
					m_serviceStatusCollectionCompPtr,
					"ServiceStatusCollection",
					"Optional status mirror (server). Leave unset on the agent — process status lives in ServiceController.",
					false,
					"ServiceStatusCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

private:
	void PublishStatusPayload(
				const QByteArray& serviceId,
				agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus) const;

	// Best-effort only; no-op when ServiceStatusCollection is not assigned.
	void UpdateOptionalStatusCollection(
				const QByteArray& serviceId,
				agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus) const;

protected:
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
};


} // namespace agentinogql
