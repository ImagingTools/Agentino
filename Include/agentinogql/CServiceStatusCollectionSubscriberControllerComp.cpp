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

	const QSet<int> changeIds = changeSet.GetIds();
	const bool relevant =
				changeIds.contains(imtbase::ICollectionInfo::CF_ADDED)
				|| changeIds.contains(imtbase::ICollectionInfo::CF_ELEMENT_STATE)
				|| changeIds.contains(imtbase::ICollectionInfo::CF_REMOVED)
				|| changeIds.contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED);
	if (!relevant){
		return;
	}

	// Do NOT extract a single serviceId from the changeSet: a batched status write (the
	// reconnect reconcile wraps all its per-service writes in one istd::CChangeGroup)
	// coalesces into a single changeSet whose CN_OBJECT_DATA_CHANGED key names only the LAST
	// service of the batch. Publishing that one id dropped every other service - reproduced
	// live: after an agent reconnect only one of two open services' status reached the GUI,
	// and the WS channel carried messages for a single serviceid only. Publish every current
	// service's status instead so a coalesced batch cannot lose any; the client filters by
	// serviceid, so the extra messages are harmless.
	const imtbase::ICollectionInfo::Ids ids = m_objectCollectionCompPtr->GetElementIds();
	for (const QByteArray& serviceId : ids){
		PublishServiceStatus(serviceId);
	}
}


void CServiceStatusCollectionSubscriberControllerComp::PublishServiceStatus(const QByteArray& serviceId)
{
	if (serviceId.isEmpty() || !m_serviceCompositeInfoCompPtr.IsValid()){
		return;
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


