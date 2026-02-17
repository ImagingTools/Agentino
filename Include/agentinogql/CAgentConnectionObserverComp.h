// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <icomp/CComponentBase.h>
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtcom/IConnectionStatusProvider.h>

// Agentino includes
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


class CAgentConnectionObserverComp:
			public icomp::CComponentBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef icomp::CComponentBase BaseClass;
	typedef imod::TSingleModelObserverBase<istd::IChangeable> BaseClass2;

	I_BEGIN_COMPONENT(CAgentConnectionObserverComp);
		I_ASSIGN(m_loginStatusProviderCompPtr, "LoginStatusProvider", "Login status provider", true, "LoginStatusProvider");
		I_ASSIGN_TO(m_loginStatusModelCompPtr, m_loginStatusProviderCompPtr, true);
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection", false, "AgentStatusCollection");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", false, "AgentCollection");
	I_END_COMPONENT;

protected:
	void SetAgentStatus(const QByteArray& agentId, agentinodata::IAgentStatusInfo::AgentStatus status) const;
	void SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status) const;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

protected:
	I_REF(imtcom::IConnectionStatusProvider, m_loginStatusProviderCompPtr);
	I_REF(imod::IModel, m_loginStatusModelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentinogql


