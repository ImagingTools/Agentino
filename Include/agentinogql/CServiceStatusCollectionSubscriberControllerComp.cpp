#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>


namespace agentinogql
{


// reimplemented (imod::CSingleModelObserverBase)

void CServiceStatusCollectionSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_objectCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		return;
	}

	QSet<int> changeIds = changeSet.GetIds();

	QByteArray serviceId;
	if (changeIds.contains(imtbase::ICollectionInfo::CF_ADDED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_INSERTED).toByteArray();
	}

	if (changeIds.contains(imtbase::ICollectionInfo::CF_ELEMENT_STATE)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_STATE).toByteArray();
	}

	if (changeIds.contains(imtbase::ICollectionInfo::CF_REMOVED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_REMOVED).toByteArray();
	}

	if (changeIds.contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_OBJECT_DATA_CHANGED).toByteArray();
	}

	QString dependencyData;
	QByteArrayList dependencyServices = m_serviceCompositeInfoCompPtr->GetDependencyServices(serviceId);
	dependencyData = "[{\"id\":\"";
	dependencyData += serviceId + "\",";
	dependencyData += "\"dependencyStatus\":\"";
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceId);
	dependencyData += agentinodata::IServiceCompositeInfo::ToString(state) + "\"}";
	for (int index = 0; index < dependencyServices.count(); index++){
		dependencyData += ",";
		dependencyData += "{\"id\":\"";
		dependencyData += dependencyServices[index] + "\",";
		dependencyData += "\"dependencyStatus\":\"";
		state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(dependencyServices[index]);
		dependencyData += agentinodata::IServiceCompositeInfo::ToString(state) + "\"}";
	}
	dependencyData += "]";

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\", \"dependencyStatus\": %3}")
					   .arg(qPrintable(serviceId)).arg(qPrintable(agentinodata::IServiceStatusInfo::ToString(status))).arg(qPrintable(dependencyData));

	SetAllSubscriptions("OnServiceStatusChanged", data.toUtf8());
}


} // namespace agentinogql


