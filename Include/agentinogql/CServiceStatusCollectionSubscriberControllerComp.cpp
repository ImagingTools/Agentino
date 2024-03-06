#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>


// Agentino includes
#include <agentinodata/CServiceStatusInfo.h>


namespace agentinogql
{


// reimplemented (imod::CSingleModelObserverBase)

void CServiceStatusCollectionSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return;
	}

	if (
			changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED) ||
			changeSet.Contains(imtbase::ICollectionInfo::CF_UPDATED) ||
			changeSet.Contains(imtbase::ICollectionInfo::CF_REMOVED)){
	}

	QSet<int> changeIds = changeSet.GetIds();

	QByteArray serviceId;
	if (changeIds.contains(imtbase::ICollectionInfo::CF_ADDED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_INSERTED).toByteArray();
	}

	if (changeIds.contains(imtbase::ICollectionInfo::CF_UPDATED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_UPDATED).toByteArray();
	}

	if (changeIds.contains(imtbase::ICollectionInfo::CF_REMOVED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_REMOVED).toByteArray();
	}

	agentinodata::CServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::CServiceStatusInfo::SS_UNDEFINED;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
		if (serviceStatusInfoPtr != nullptr){
			serviceStatus = serviceStatusInfoPtr->GetServiceStatus();
		}
	}

	agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(serviceStatus);

	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\" }").arg(serviceId).arg(processStateEnum.id);

	SetAllSubscriptions("OnServiceStatusChanged", data.toUtf8());
}


} // namespace agentinogql


