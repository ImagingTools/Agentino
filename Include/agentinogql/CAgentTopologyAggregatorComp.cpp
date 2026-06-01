// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CAgentTopologyAggregatorComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentTopology.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


// public methods

QList<CAgentTopologyAggregatorComp::AgentTopologyCache> CAgentTopologyAggregatorComp::GetAggregatedTopology() const
{
	QMutexLocker locker(&m_cacheMutex);
	return m_topologyCache.values();
}


void CAgentTopologyAggregatorComp::UpdateAgentTopology(
			const QByteArray& agentId,
			const sdl::V1_0::agentino::CLocalTopology& localTopology)
{
	QMutexLocker locker(&m_cacheMutex);

	AgentTopologyCache& cache = m_topologyCache[agentId];
	cache.agentId = agentId;

	if (localTopology.agentName.has_value()){
		cache.agentName = *localTopology.agentName;
	}

	cache.connectionStatus = sdl::V1_0::agentino::AgentConnectionStatus::ONLINE;
	cache.lastSeen = QDateTime::currentDateTime();

	cache.services.clear();
	if (localTopology.services.has_value()){
		for (const istd::TNullableValue<sdl::V1_0::agentino::CLocalServiceInfo>& service : **localTopology.services){
			if (service.has_value()){
				cache.services << *service;
			}
		}
	}

	// Synchronize service status into the central ServiceStatusCollection
	if (m_serviceStatusCollectionCompPtr.IsValid()){
		for (const sdl::V1_0::agentino::CLocalServiceInfo& serviceInfo : cache.services){
			if (!serviceInfo.id.has_value()){
				continue;
			}

			QByteArray serviceId = *serviceInfo.id;
			agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::IServiceStatusInfo::SS_UNDEFINED;

			if (serviceInfo.status.has_value()){
				sdl::V1_0::agentino::LocalServiceStatus localStatus = *serviceInfo.status;
				if (localStatus == sdl::V1_0::agentino::LocalServiceStatus::RUNNING){
					serviceStatus = agentinodata::IServiceStatusInfo::SS_RUNNING;
				}
				else if (localStatus == sdl::V1_0::agentino::LocalServiceStatus::NOT_RUNNING){
					serviceStatus = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
				}
				else if (localStatus == sdl::V1_0::agentino::LocalServiceStatus::STARTING){
					serviceStatus = agentinodata::IServiceStatusInfo::SS_STARTING;
				}
				else if (localStatus == sdl::V1_0::agentino::LocalServiceStatus::STOPPING){
					serviceStatus = agentinodata::IServiceStatusInfo::SS_STOPPING;
				}
				else if (localStatus == sdl::V1_0::agentino::LocalServiceStatus::RUNNING_IMPOSSIBLE){
					serviceStatus = agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE;
				}
			}

			istd::TDelPtr<agentinodata::CServiceStatusInfo> statusInfoPtr;
			statusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);
			statusInfoPtr->SetServiceId(serviceId);
			statusInfoPtr->SetServiceStatus(serviceStatus);

			imtbase::ICollectionInfo::Ids elementIds = m_serviceStatusCollectionCompPtr->GetElementIds();
			if (elementIds.contains(serviceId)){
				m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *statusInfoPtr.PopPtr());
			}
			else{
				m_serviceStatusCollectionCompPtr->InsertNewObject(
					"ServiceStatusInfo", "", "", statusInfoPtr.PopPtr(), serviceId);
			}
		}
	}
}


void CAgentTopologyAggregatorComp::SetAgentOffline(const QByteArray& agentId)
{
	QMutexLocker locker(&m_cacheMutex);

	if (m_topologyCache.contains(agentId)){
		m_topologyCache[agentId].connectionStatus = sdl::V1_0::agentino::AgentConnectionStatus::OFFLINE;
	}
}


void CAgentTopologyAggregatorComp::SetAgentOnline(const QByteArray& agentId)
{
	QMutexLocker locker(&m_cacheMutex);

	if (m_topologyCache.contains(agentId)){
		m_topologyCache[agentId].connectionStatus = sdl::V1_0::agentino::AgentConnectionStatus::ONLINE;
		m_topologyCache[agentId].lastSeen = QDateTime::currentDateTime();
	}
}


void CAgentTopologyAggregatorComp::RemoveAgent(const QByteArray& agentId)
{
	QMutexLocker locker(&m_cacheMutex);
	m_topologyCache.remove(agentId);
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentTopologyAggregatorComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	QByteArray clientId = changeSet.GetChangeInfo("ClientId").toByteArray();
	if (clientId.isEmpty()){
		return;
	}

	if (changeSet.Contains(imtcom::IConnectionStatusProvider::CS_CONNECTED)){
		SetAgentOnline(clientId);
	}
	else{
		SetAgentOffline(clientId);
	}
}


// reimplemented (icomp::CComponentBase)

void CAgentTopologyAggregatorComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_loginStatusModelCompPtr.IsValid()){
		m_loginStatusModelCompPtr->AttachObserver(this);
	}
}


void CAgentTopologyAggregatorComp::OnComponentDestroyed()
{
	if (m_loginStatusModelCompPtr.IsValid()){
		m_loginStatusModelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


} // namespace agentinogql
