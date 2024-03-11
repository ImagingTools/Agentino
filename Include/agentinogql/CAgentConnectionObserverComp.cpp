#include <agentinogql/CAgentConnectionObserverComp.h>


// Agentino includes
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentInfo.h>


namespace agentinogql
{


// protected methods

// reimplemented (imod::CSingleModelObserverBase)

void CAgentConnectionObserverComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	QByteArray clientId = changeSet.GetChangeInfo("ClientId").toByteArray();

	if (changeSet.Contains(0)){
		if (m_agentCollectionCompPtr.IsValid() && m_serviceStatusCollectionCompPtr.IsValid()){
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


