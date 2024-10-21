#include <agentinogql/CServiceSubscriberControllerComp.h>


// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


// protected methods

bool CServiceSubscriberControllerComp::SetSubscriptions()
{
	return true;
}


// reimplemented (icomp::CComponentBase)

void CServiceSubscriberControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()) {
		m_modelCompPtr->AttachObserver(this);
	}
}

void CServiceSubscriberControllerComp::OnComponentDestroyed()
{
	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


// reimplemented (imod::CSingleModelObserverBase)

void CServiceSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	agentinodata::IServiceController::NotifierStatusInfo notifierStatusInfo = changeSet.GetChangeInfo(
				agentinodata::IServiceController::CN_STATUS_CHANGED).value<agentinodata::IServiceController::NotifierStatusInfo>();
	QString status;
	status = agentinodata::IServiceStatusInfo::ToString(notifierStatusInfo.serviceStatus);

	QByteArray serviceId = notifierStatusInfo.serviceId;

	if (serviceId.isEmpty()){
		return;
	}

	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\" }").arg(qPrintable(notifierStatusInfo.serviceId)).arg(status);
	SetAllSubscriptions("OnAgentServiceStatusChanged", data.toUtf8());

	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
			agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				serviceStatusInfoPtr->SetServiceStatus(notifierStatusInfo.serviceStatus);
				m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr);
			}
		}
	}
}


} // namespace agentinogql


