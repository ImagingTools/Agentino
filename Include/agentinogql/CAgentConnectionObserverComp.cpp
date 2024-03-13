#include <agentinogql/CAgentConnectionObserverComp.h>


// Agentino includes
#include <agentinodata/CAgentStatusInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentInfo.h>


namespace agentinogql
{


// protected methods

// reimplemented (imod::CSingleModelObserverBase)

void CAgentConnectionObserverComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_agentStatusCollectionCompPtr.IsValid()){
		return;
	}

	if (!m_agentCollectionCompPtr.IsValid()){
		return;
	}

	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		return;
	}

	QByteArray clientId = changeSet.GetChangeInfo("ClientId").toByteArray();

	if (changeSet.Contains(0)){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(clientId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids elementIds = serviceCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
						istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
						serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

						serviceStatusInfoPtr->SetServiceId(elementId);
						serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_UNDEFINED);

						m_serviceStatusCollectionCompPtr->SetObjectData(elementId, *serviceStatusInfoPtr.PopPtr());
					}
				}
			}
		}
	}

	if (changeSet.Contains(imtauth::ILoginStatusProvider::LSF_LOGGED_IN)){
		istd::TDelPtr<agentinodata::CAgentStatusInfo> serviceStatusInfoPtr;
		serviceStatusInfoPtr.SetPtr(new agentinodata::CAgentStatusInfo(clientId, agentinodata::IAgentStatusInfo::AS_CONNECTED));

		imtbase::ICollectionInfo::Ids elementIds = m_agentStatusCollectionCompPtr->GetElementIds();
		if (elementIds.contains(clientId)){
			m_agentStatusCollectionCompPtr->SetObjectData(clientId, *serviceStatusInfoPtr.PopPtr());
		}
		else{
			m_agentStatusCollectionCompPtr->InsertNewObject("AgentStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), clientId);
		}
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentConnectionObserverComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_loginStatusModelCompPtr.IsValid()) {
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


