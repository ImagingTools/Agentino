#include <agentgql/CServiceSubscriberControllerComp.h>


// ImtCore includes
#include <imtrest/IProtocolEngine.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentinodata/CServiceStatusInfo.h>


namespace agentgql
{


// protected methods

bool CServiceSubscriberControllerComp::SetSubscriptions()
{
	if (!m_requestManagerCompPtr.IsValid()){
		return false;
	}

	for (RequestNetworks& requestNetworks: m_registeredSubscribers){
		for (const QByteArray& id: requestNetworks.networkRequests.keys()){
			const imtrest::IRequest* networkRequest = requestNetworks.networkRequests[id];
			QByteArray body = QString(R"(
										{
											"type": "data",
											"id": %1,
											"payload": {
												"data": %2
											}
										})")
								.arg(QString(id))
								.arg(QString("Test")).toUtf8();
			QByteArray reponseTypeId = QByteArray("application/json; charset=utf-8");
			const imtrest::IProtocolEngine& engine = networkRequest->GetProtocolEngine();

			imtrest::ConstResponsePtr responsePtr(engine.CreateResponse(*networkRequest, imtrest::IProtocolEngine::SC_OPERATION_NOT_AVAILABLE, body, reponseTypeId));
			if (responsePtr.IsValid()){
				const imtrest::ISender* sender = m_requestManagerCompPtr->GetSender(networkRequest->GetRequestId());
				if (sender != nullptr){
					sender->SendResponse(responsePtr);
				}
			}
		}
	}

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
	status = QVariant::fromValue(notifierStatusInfo.serviceStatus).toString();

	QByteArray serviceId = notifierStatusInfo.serviceId;
	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\" }").arg(QString(notifierStatusInfo.serviceId)).arg(status);
	SetAllSubscriptions("OnServiceStateChanged", data.toUtf8());

	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
			agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				if (status == "Running"){
					serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_RUNNING);
				}
				else if (status == "NotRunning"){
					serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
				}
				else{
					serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_NONE);
				}

				m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr);
			}
		}
	}
}


} // namespace agentgql

