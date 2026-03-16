// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceStatusCollectionSubscriberControllerComp.h>


// Agentino includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


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
	else if (changeIds.contains(imtbase::ICollectionInfo::CF_ELEMENT_STATE)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_ELEMENT_STATE).toByteArray();
	}
	else if (changeIds.contains(imtbase::ICollectionInfo::CF_REMOVED)){
		QVariant changeInfo = changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENTS_REMOVED);
		if (changeInfo.isValid()){
			imtbase::ICollectionInfo::MultiElementNotifierInfo info = changeInfo.value<imtbase::ICollectionInfo::MultiElementNotifierInfo>();
			if (!info.elementIds.isEmpty()){
				serviceId = info.elementIds[0];
			}
		}
	}
	else if (changeIds.contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED)){
		serviceId = changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_OBJECT_DATA_CHANGED).toByteArray();
	}

	QString dependencyData;
	QByteArrayList dependencyServices = m_serviceCompositeInfoCompPtr->GetDependencyServices(serviceId);
	dependencyData = "[{\"id\":\"";
	dependencyData += serviceId + "\",";
	dependencyData += "\"dependencyStatus\":\"";
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceId);
	dependencyData += GetDependencyStatus(state) + "\"}";
	for (int index = 0; index < dependencyServices.count(); index++){
		dependencyData += ",";
		dependencyData += "{\"id\":\"";
		dependencyData += dependencyServices[index] + "\",";
		dependencyData += "\"dependencyStatus\":\"";

		state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(dependencyServices[index]);
		QString statusStr = GetDependencyStatus(state);

		dependencyData += statusStr + "\"}";
	}
	dependencyData += "]";

	QString statusStr;
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	if (status == agentinodata::IServiceStatusInfo::ServiceStatus::SS_RUNNING){
		statusStr = "RUNNING";
	}
	else if (status == agentinodata::IServiceStatusInfo::ServiceStatus::SS_NOT_RUNNING){
		statusStr = "NOT_RUNNING";
	}
	else if (status == agentinodata::IServiceStatusInfo::ServiceStatus::SS_STARTING){
		statusStr = "STARTING";
	}
	else if (status == agentinodata::IServiceStatusInfo::ServiceStatus::SS_STOPPING){
		statusStr = "STOPPING";
	}
	else if (status == agentinodata::IServiceStatusInfo::ServiceStatus::SS_RUNNING_IMPOSSIBLE){
		statusStr = "RUNNING_IMPOSSIBLE";
	}
	else{
		statusStr = "UNDEFINED";
	}

	QString data = QString("{ \"serviceid\": \"%1\", \"serviceStatus\": \"%2\", \"dependencyStatus\": %3}")
					   .arg(qPrintable(serviceId)).arg(statusStr).arg(qPrintable(dependencyData));

	PublishData("OnServiceStatusChanged", data.toUtf8());
}


// private methods

QString CServiceStatusCollectionSubscriberControllerComp::GetDependencyStatus(agentinodata::IServiceCompositeInfo::StateOfRequiredServices status) const
{
	QString statusStr;

	if (status == agentinodata::IServiceCompositeInfo::StateOfRequiredServices::SORS_RUNNING){
		statusStr = "RUNNING";
	}
	else if (status == agentinodata::IServiceCompositeInfo::StateOfRequiredServices::SORS_NOT_RUNNING){
		statusStr = "NOT_RUNNING";
	}
	else{
		statusStr = "UNDEFINED";
	}

	return statusStr;
}


} // namespace agentinogql


