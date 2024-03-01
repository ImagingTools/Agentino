#include <agentinodata/CAgentCollectionComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentinodata
{


// reimplemented (agentinodata::IServiceManager)

bool CAgentCollectionComp::AddService(const QByteArray& agentId, const IServiceInfo& serviceInfo)
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->objectPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				QByteArray objectId = serviceCollectionPtr->InsertNewObject("ServiceInfo", "", "", &serviceInfo);
				if (!objectId.isEmpty()){
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_ADDED);
					changeSet.SetChangeInfo("agentId", agentId);
					changeSet.SetChangeInfo("serviceId", objectId);

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
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->objectPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				bool result = serviceCollectionPtr->RemoveElement(serviceId);
				if (result){
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_REMOVED);
					changeSet.SetChangeInfo("agentId", agentId);
					changeSet.SetChangeInfo("serviceId", serviceId);

					istd::CChangeNotifier changeNotifier(this, &changeSet);
				}

				return result;
			}
		}
	}

	return false;
}


bool CAgentCollectionComp::SetService(const QByteArray& agentId, const QByteArray& serviceId, const IServiceInfo& serviceInfo)
{
	ObjectInfo* objectInfoPtr = GetObjectInfo(agentId);
	if (objectInfoPtr != nullptr){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->objectPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				bool result = serviceCollectionPtr->SetObjectData(serviceId, serviceInfo);
				if (result){
					ChangeSet changeSet(agentinodata::IServiceManager::CF_SERVICE_UPDATED);
					changeSet.SetChangeInfo("agentId", agentId);
					changeSet.SetChangeInfo("serviceId", serviceId);

					istd::CChangeNotifier changeNotifier(this, &changeSet);
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
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(objectInfoPtr->objectPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr != nullptr){
				return serviceCollectionPtr->GetElementIds().contains(serviceId);
			}
		}
	}

	return false;
}


} // namespace agentinodata


