#include <agentinogql/CServiceCollectionSubscriberControllerComp.h>


// ImtCore includes
#include<imtrest/IProtocolEngine.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>


namespace agentinogql
{


// reimplemented (imod::CSingleModelObserverBase)

void CServiceCollectionSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_requestManagerCompPtr.IsValid()){
		return;
	}

	if (
			changeSet.Contains(agentinodata::IServiceManager::CF_SERVICE_ADDED) ||
			changeSet.Contains(agentinodata::IServiceManager::CF_SERVICE_UPDATED) ||
			changeSet.Contains(agentinodata::IServiceManager::CF_SERVICE_REMOVED)){
		QByteArray agentId = changeSet.GetChangeInfo("agentId").toByteArray();
		QByteArray serviceId = changeSet.GetChangeInfo("serviceId").toByteArray();

		for (const RequestNetworks& requestNetworks: m_registeredSubscribers){
			for (const QByteArray& id: requestNetworks.networkRequests.keys()){
				const imtrest::IRequest* networkRequest = requestNetworks.networkRequests[id];
				QByteArray body = QString(R"({"type": "data", "id": "%1", "payload": {"data": {"agentId": "%2", "serviceId": "%3"}}})")
							.arg(qPrintable(id))
							.arg(qPrintable(agentId))
							.arg(qPrintable(serviceId)).toUtf8();

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
	}
}


} // namespace agentinogql


