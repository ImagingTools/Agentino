#include <agentinodata/CAgentCollectionComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentinodata
{


// reimplemented (agentinodata::IServiceManager)

bool CAgentCollectionComp::AddService(
	const QByteArray& agentId,
	const IServiceInfo& serviceInfo,
	const QByteArray& serviceId,
	const QString& serviceName,
	const QString& serviceDescription)
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				QByteArray objectId = serviceCollectionPtr->InsertNewObject("ServiceInfo", serviceName, serviceDescription, &serviceInfo, serviceId);
				if (!objectId.isEmpty()){
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_ADDED);
					changeSet.SetChangeInfo("agentid", agentId);
					changeSet.SetChangeInfo("serviceid", objectId);
					
					istd::CChangeNotifier changeNotifier(this, &changeSet);
				}
				
				return !objectId.isEmpty();
			}
		}
	}
	
	return false;
}


bool CAgentCollectionComp::RemoveService(const QByteArray& agentId, const QByteArray& serviceId)
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				imtbase::ICollectionInfo::Ids objectIds;
				objectIds << serviceId;

				bool result = serviceCollectionPtr->RemoveElements(objectIds);
				if (result){
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_REMOVED);
					changeSet.SetChangeInfo("agentid", agentId);
					changeSet.SetChangeInfo("serviceid", serviceId);
					
					istd::CChangeNotifier changeNotifier(this, &changeSet);
				}
				
				return result;
			}
		}
	}
	
	return false;
}


bool CAgentCollectionComp::SetService(
	const QByteArray& agentId,
	const QByteArray& serviceId,
	const IServiceInfo& serviceInfo,
	const QString& serviceName,
	const QString& serviceDescription,
	bool beQuiet)
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				bool result = serviceCollectionPtr->SetObjectData(serviceId, serviceInfo);
				if (result){
					serviceCollectionPtr->SetElementName(serviceId, serviceName);
					serviceCollectionPtr->SetElementDescription(serviceId, serviceDescription);
					
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_UPDATED);
					changeSet.SetChangeInfo("agentid", agentId);
					changeSet.SetChangeInfo("serviceid", serviceId);
					
					if (!beQuiet){
						istd::CChangeNotifier changeNotifier(this, &changeSet);
					}
				}
				
				return result;
			}
		}
	}
	
	return false;
}


bool CAgentCollectionComp::ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				return serviceCollectionPtr->GetElementIds().contains(serviceId);
			}
		}
	}
	
	return false;
}


IServiceInfo* CAgentCollectionComp::GetService(const QByteArray& agentId, const QByteArray& serviceId) const
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr == nullptr){
		return nullptr;
	}
	
	agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->dataPtr.GetPtr());
	if (agentInfoPtr == nullptr){
		return nullptr;
	}
	
	imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
	if (serviceCollectionPtr == nullptr){
		return nullptr;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!serviceCollectionPtr->GetObjectData(serviceId, dataPtr)){
		return nullptr;
	}
	
	istd::TDelPtr<IServiceInfo> serviceInfoPtr;
	if (!serviceInfoPtr.SetCastedOrRemove(dataPtr.GetPtr()->CloneMe())){
		return nullptr;
	}
	
	return serviceInfoPtr.PopPtr();
}


} // namespace agentinodata


