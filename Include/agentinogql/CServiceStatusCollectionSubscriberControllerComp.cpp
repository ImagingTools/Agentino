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

	if (changeIds.contains(imtbase::ICollectionInfo::CF_UPDATED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_UPDATED).toByteArray();
	}

	if (changeIds.contains(imtbase::ICollectionInfo::CF_REMOVED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_REMOVED).toByteArray();
	}

	QString dependencyData;
	QByteArrayList dependencyServices = m_serviceCompositeInfoCompPtr->GetDependencyServices(serviceId);
	dependencyData = "[{\"id\":\"";
	dependencyData += serviceId + "\",";
	dependencyData += "\"dependencyStatus\":\"";
	dependencyData += m_serviceCompositeInfoCompPtr->GetDependantServiceStatus(serviceId) + "\"}";
	for (int index = 0; index < dependencyServices.count(); index++){
		dependencyData += ",";
		dependencyData += "{\"id\":\"";
		dependencyData += dependencyServices[index] + "\",";
		dependencyData += "\"dependencyStatus\":\"";
		dependencyData += m_serviceCompositeInfoCompPtr->GetDependantServiceStatus(dependencyServices[index]) + "\"}";
	}
	dependencyData += "]";

	QString status = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\", \"dependencyStatus\": %3}")
					   .arg(qPrintable(serviceId)).arg(status).arg(qPrintable(dependencyData));

	SetAllSubscriptions("OnServiceStatusChanged", data.toUtf8());
}


} // namespace agentinogql


