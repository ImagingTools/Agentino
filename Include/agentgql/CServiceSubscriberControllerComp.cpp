#include <agentgql/CServiceSubscriberControllerComp.h>


// ImtCore includes
#include <imtrest/IProtocolEngine.h>

// ServiceManajer includes
#include <agentinodata/IServiceController.h>


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
//	IServiceController::NotifierStatusInfo notifierStatusInfo;
//	notifierStatusInfo.serviceId = serviceId;
//	notifierStatusInfo.serviceStatus = newState;
//	changeSet.SetChangeInfo(IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierStatusInfo));

	agentinodata::IServiceController::NotifierStatusInfo notifierStatusInfo = changeSet.GetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED).value<agentinodata::IServiceController::NotifierStatusInfo>();
	QString status;
	status = QVariant::fromValue(notifierStatusInfo.serviceStatus).toString();
	QString data = QString("{ \"serviceId\": \"%1\", \"serviceStatus\": \"%2\" }").arg(QString(notifierStatusInfo.serviceId)).arg(status);
	SetAllSubscriptions("OnServiceStateChanged", data.toUtf8());
}


} // namespace agentgql

