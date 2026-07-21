// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/ServiceEndpointId.h>
#include <agentinodata/CServiceCompositeInfoComp.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>



namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceCompositeInfo)

QByteArray CServiceCompositeInfoComp::GetServiceId(const QUrl& url) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			IAgentInfo* agentInfoPtr = dynamic_cast<IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}
			// Services live in the ServiceManager mirror, not inside the agent record.
			imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(elementId)
							: nullptr;
			if (serviceCollectionPtr == nullptr){
				continue;
			}
			QByteArray serviceId = FindServiceIdByUrl(*serviceCollectionPtr, url);
			if (!serviceId.isEmpty()){
				return serviceId;
			}
		}
	}

	return QByteArray();
}


QByteArray CServiceCompositeInfoComp::GetServiceId(const QByteArray& dependantServiceConnectionId) const
{
	// Service ids are unique fleet-wide, and the endpoint id carries the producing one,
	// so the server does not have to search its agents to answer this.
	return ServiceEndpointId::ServiceOf(dependantServiceConnectionId);
}


IServiceStatusInfo::ServiceStatus CServiceCompositeInfoComp::GetServiceStatus(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus retVal = IServiceStatusInfo::SS_UNDEFINED;
	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr serviceStatusDataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, serviceStatusDataPtr)){
			IServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<IServiceStatusInfo*>(serviceStatusDataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				retVal = serviceStatusInfoPtr->GetServiceStatus();
			}
		}
	}

	return retVal;
}


IServiceCompositeInfo::StateOfRequiredServices CServiceCompositeInfoComp::GetStateOfRequiredServices(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus serviceStatus = GetServiceStatus(serviceId);
	if (serviceStatus == IServiceStatusInfo::SS_UNDEFINED || serviceStatus == IServiceStatusInfo::SS_NOT_RUNNING){
		return SORS_UNDEFINED;
	}

	StateOfRequiredServices retVal = SORS_RUNNING;

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			IAgentInfo* agentInfoPtr = dynamic_cast<IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(elementId)
							: nullptr;
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
							IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr == nullptr){
								continue;
							}
							retVal = CalculateStateOfRequiredServices(*serviceInfoPtr);
						}

						break;
					}
				}
			}
		}
	}

	return retVal;
}


QByteArrayList CServiceCompositeInfoComp::GetDependencyServices(const QByteArray& serviceId) const
{
	QByteArrayList retVal;

	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArrayList();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			IAgentInfo* agentInfoPtr = dynamic_cast<IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(elementId)
							: nullptr;
				if (serviceCollectionPtr != nullptr){
					CollectDependencyServices(*serviceCollectionPtr, serviceId, retVal);
				}
			}
		}
	}

	return retVal;
}


QString CServiceCompositeInfoComp::GetServiceName(const QByteArray& serviceId) const
{
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(elementId)
							: nullptr;
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						imtbase::IObjectCollection::DataPtr dataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceId, dataPtr)){
							IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(dataPtr.GetPtr());
							if (serviceInfoPtr != nullptr){
								return serviceInfoPtr->GetServiceName();
							}
						}
					}
				}
			}
		}
	}

	return QString();
}


QString CServiceCompositeInfoComp::GetServiceAgentName(const QByteArray& serviceId) const
{
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			IAgentInfo* agentInfoPtr = dynamic_cast<IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(elementId)
							: nullptr;
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();

						return agentName;
					}
				}
			}
		}
	}

	return QString();
}


} // namespace agentinodata


