#include <agentinogql/CAgentConnectionObserverComp.h>


// Agentino includes
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentInfo.h>

// Qt includes
#include <QtCore/QDebug>


namespace agentinogql
{


// protected methods

void CAgentConnectionObserverComp::SetAgentStatus(
			const QByteArray& agentId,
			agentinodata::IAgentStatusInfo::AgentStatus status) const
{
	if (!m_agentStatusCollectionCompPtr.IsValid()){
		return;
	}

	istd::TDelPtr<agentinodata::CAgentStatusInfo> serviceStatusInfoPtr;
	serviceStatusInfoPtr.SetPtr(new agentinodata::CAgentStatusInfo(agentId, status));

	imtbase::ICollectionInfo::Ids elementIds = m_agentStatusCollectionCompPtr->GetElementIds();
	if (elementIds.contains(agentId)){
		m_agentStatusCollectionCompPtr->SetObjectData(agentId, *serviceStatusInfoPtr.PopPtr());
	}
	else{
		m_agentStatusCollectionCompPtr->InsertNewObject("AgentStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), agentId);
	}
}


void CAgentConnectionObserverComp::SetServiceStatus(
			const QByteArray& serviceId,
			agentinodata::IServiceStatusInfo::ServiceStatus /*status*/) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		return;
	}

	istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
	serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

	serviceStatusInfoPtr->SetServiceId(serviceId);
	serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_UNDEFINED);

	m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr.PopPtr());
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentConnectionObserverComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return;
	}

	QByteArray clientId = changeSet.GetChangeInfo("ClientId").toByteArray();
	if (clientId.isEmpty()){
		return;
	}

	if (changeSet.Contains(imtcom::IConnectionStatusProvider::CS_CONNECTED)){
		SetAgentStatus(clientId, agentinodata::IAgentStatusInfo::AS_CONNECTED);
	}
	else{
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(clientId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids elementIds = serviceCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
						SetServiceStatus(elementId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
					}
				}
			}
		}

		SetAgentStatus(clientId, agentinodata::IAgentStatusInfo::AS_DISCONNECTED);
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentConnectionObserverComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_loginStatusModelCompPtr.IsValid()){
		m_loginStatusModelCompPtr->AttachObserver(this);
	}
}


void CAgentConnectionObserverComp::OnComponentDestroyed()
{
	if (m_loginStatusModelCompPtr.IsValid()){
		m_loginStatusModelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql


