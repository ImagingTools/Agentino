#include <agentinogql/CServiceSubscriberControllerComp.h>


// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


// protected methods

// reimplemented (icomp::CComponentBase)

void CServiceSubscriberControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	qDebug() << "CServiceSubscriberControllerComp OnComponentCreated";

	if (m_modelCompPtr.IsValid()){
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

	QString data = QString("{ \"serviceid\": \"%1\", \"serviceStatus\": \"%2\" }").arg(qPrintable(notifierStatusInfo.serviceId)).arg(status);
	if (m_commandIdsAttrPtr.GetCount() > 0){
		PublishData(m_commandIdsAttrPtr[0], data.toUtf8());
	}

	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
			agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				serviceStatusInfoPtr->SetServiceStatus(notifierStatusInfo.serviceStatus);
				if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr)){
					SendErrorMessage(0,
									 QString("Unable to set status '%1' for service '%2'. Error: internal error").arg(status).arg(qPrintable(notifierStatusInfo.serviceId)),
									 "CServiceSubscriberControllerComp");
				}
			}
		}
	}
}


} // namespace agentinogql


