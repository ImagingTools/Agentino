#include <agentgql/CAgentServicesRemoteSubscriberProxyComp.h>


// Qt includes
#include <QtWebSockets/QWebSocket>

// ImtCore includes
#include <imtgql/CGqlResponse.h>
#include <imtbase/IObjectCollection.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtbase/CUrlParam.h>
#include <imtrest/IProtocolEngine.h>


namespace agentgql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)

bool CAgentServicesRemoteSubscriberProxyComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	QByteArray serviceId = gqlRequest.GetHeader("serviceid");

	return !serviceId.isEmpty();
}


bool CAgentServicesRemoteSubscriberProxyComp::RegisterSubscription(
	const QByteArray& subscriptionId,
	const imtgql::CGqlRequest& gqlRequest,
	const imtrest::IRequest& networkRequest,
	QString& /*errorMessage*/)
{
	if (!IsRequestSupported(gqlRequest)){
		return false;
	}

	if (!m_connectionCollectionProviderCompPtr.IsValid()){
		return false;
	}

	istd::TSharedInterfacePtr<imtgql::IGqlRequest> gqlRequestPtr;
	gqlRequestPtr.MoveCastedPtr(gqlRequest.CloneMe());
	if (!gqlRequestPtr.IsValid()){
		return false;
	}

	QByteArray serviceId = gqlRequest.GetHeader("serviceid");
	QUrl url;
	QByteArray serviceTypeName;

	imtservice::IConnectionCollectionPlugin::IConnectionCollectionSharedPtr connectionCollectionPtr = m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServiceId(serviceId);
	if (!connectionCollectionPtr.IsValid()){
		return false;
	}

	const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollectionPtr->GetServerConnectionList());
	const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
	if (objectCollection != nullptr){
		imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
		for (const QByteArray& id: ids){
			const imtservice::IServiceConnectionInfo* connectionParamPtr = connectionCollectionPtr->GetConnectionMetaInfo(id);
			if (connectionParamPtr == nullptr){
				continue;
			}

			if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
				serviceTypeName = connectionCollectionPtr->GetServiceTypeId().toUtf8();
				const imtcom::IServerConnectionInterface& serverConnectionInterface = connectionParamPtr->GetDefaultInterface();
				if (serverConnectionInterface.GetUrl(imtcom::IServerConnectionInterface::PT_WEBSOCKET, url)){
					break;
				}
			}
		}
	}

	if (!m_webSocketClientMap.contains(serviceId)){
		istd::TDelPtr<QWebSocket> webSocketClientPtr(new QWebSocket());
		webSocketClientPtr->open(url);
		NetworkOperation networkOperation(100, this);
		for (int i = 0; i < 100; i++){
			networkOperation.timer.start();
			networkOperation.connectionLoop.exec(QEventLoop::ExcludeUserInputEvents);
			QCoreApplication::processEvents();
			if (webSocketClientPtr->state() == QAbstractSocket::ConnectedState){
				break;
			}
		}
		if (webSocketClientPtr->state() == QAbstractSocket::ConnectedState){
			QWebSocket* websocketPtr = webSocketClientPtr.PopPtr();
			connect(websocketPtr, &QWebSocket::connected, this, &CAgentServicesRemoteSubscriberProxyComp::OnWebSocketConnected, Qt::QueuedConnection);
			connect(websocketPtr, &QWebSocket::disconnected, this, &CAgentServicesRemoteSubscriberProxyComp::OnWebSocketDisconnected, Qt::QueuedConnection);
			connect(websocketPtr, &QWebSocket::textMessageReceived, this, &CAgentServicesRemoteSubscriberProxyComp::OnWebSocketTextMessageReceived);
			connect(websocketPtr, &QWebSocket::binaryMessageReceived, this, &CAgentServicesRemoteSubscriberProxyComp::OnWebSocketBinaryMessageReceived);
			connect(websocketPtr, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(OnWebSocketError(QAbstractSocket::SocketError)));

			m_webSocketClientMap.insert(serviceId, websocketPtr);
		}
	}

	if (!m_requestMap.contains(subscriptionId)){
		RequestInfo requestInfo;
		requestInfo.serviceId = serviceId;
		requestInfo.gqlRequestPtr = gqlRequestPtr;
		requestInfo.networkRequestPtr = &networkRequest;

		m_requestMap.insert(subscriptionId, requestInfo);
	}

	if (m_webSocketClientMap.contains(serviceId)){
		QByteArray queryData = networkRequest.GetBody();
		m_webSocketClientMap.value(serviceId)->sendTextMessage(queryData);
	}

	return true;
}


bool CAgentServicesRemoteSubscriberProxyComp::UnregisterSubscription(const QByteArray& subscriptionId)
{
	BaseClass::UnregisterSubscription(subscriptionId);

	if (m_requestMap.contains(subscriptionId)){
		QByteArray serviceId = m_requestMap.value(subscriptionId).serviceId;

		if (m_webSocketClientMap.contains(serviceId)){
			m_webSocketClientMap.remove(serviceId);
		}
	}

	return true;
}


// reimplemented (imtrest::IRequestEventHandler)

void CAgentServicesRemoteSubscriberProxyComp::OnRequestDestroyed(imtrest::IRequest* request)
{
	for (auto it = m_requestMap.begin(); it != m_requestMap.end(); ++it){
		if (it.value().networkRequestPtr == request){
			m_requestMap.remove(it.key());

			break;
		}
	}
}


// private slots

void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketConnected()
{
	QWebSocket* webSocketPtr = dynamic_cast<QWebSocket*>(sender());

	if (webSocketPtr == nullptr){
		return;
	}

	QString body = "{ \"type\": \"connection_init\"}";

	webSocketPtr->sendTextMessage(body);

	const QByteArray serviceId = GetServiceId(*webSocketPtr);
	if (m_reconnectTimers.contains(serviceId)){
		m_reconnectTimers[serviceId]->stop();
		m_reconnectTimers[serviceId]->deleteLater();
		m_reconnectTimers.remove(serviceId);
	}
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketDisconnected()
{
	QWebSocket* webSocketPtr = dynamic_cast<QWebSocket*>(sender());
	if (webSocketPtr == nullptr){
		return;
	}

	const QByteArray serviceId = GetServiceId(*webSocketPtr);

	if (m_webSocketClientMap.contains(serviceId)){
		m_webSocketClientMap.remove(serviceId);
	}

	if (!m_reconnectTimers.contains(serviceId)){
		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, [this, serviceId](){
			this->TryReconnect(serviceId);
		});

		timer->start(5000);
		m_reconnectTimers[serviceId] = timer;
	}

	webSocketPtr->deleteLater();
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketError(QAbstractSocket::SocketError /*error*/)
{
	QWebSocket* webSocket = dynamic_cast<QWebSocket*>(sender());
	if (webSocket != nullptr){
		QString errorText = webSocket->errorString();

		SendErrorMessage(0, errorText, "CAgentServicesRemoteSubscriberProxyComp");
	}
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketTextMessageReceived(const QString& message)
{
	QWebSocket* webSocketPtr = dynamic_cast<QWebSocket*>(sender());

	if (webSocketPtr == nullptr){
		return;
	}

	if (!m_requestManagerCompPtr.IsValid()){
		return;
	}

	QByteArray body = message.toUtf8();
	QJsonDocument document = QJsonDocument::fromJson(body);
	QJsonObject object = document.object();

	QByteArray subscriptionId = object.value("id").toString().toUtf8();

	if (!m_requestMap.contains(subscriptionId)){
		return;
	}

	const imtrest::IRequest* networkRequest = m_requestMap.value(subscriptionId).networkRequestPtr;
	if (networkRequest == nullptr){
		return;
	}

	QByteArray reponseTypeId = QByteArray("application/json; charset=utf-8");
	const imtrest::IProtocolEngine& engine = networkRequest->GetProtocolEngine();

	imtrest::ConstResponsePtr responsePtr(
				engine.CreateResponse(
							*networkRequest,
							imtrest::IProtocolEngine::SC_OPERATION_NOT_AVAILABLE,
							body,
							reponseTypeId).PopInterfacePtr());
	if (responsePtr.IsValid()){
		const imtrest::ISender* sender = m_requestManagerCompPtr->GetSender(networkRequest->GetRequestId());
		if (sender != nullptr){
			sender->SendResponse(responsePtr);
		}
	}
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketBinaryMessageReceived(const QByteArray& /*message*/)
{
}


// private methods

void CAgentServicesRemoteSubscriberProxyComp::TryReconnect(const QByteArray& serviceId){
	if (m_webSocketClientMap.contains(serviceId)){
		if (m_reconnectTimers.contains(serviceId)){
			m_reconnectTimers[serviceId]->stop();
			m_reconnectTimers[serviceId]->deleteLater();
			m_reconnectTimers.remove(serviceId);
		}

		return;
	}

	QString errorMessage;

	for (auto it = m_requestMap.begin(); it != m_requestMap.end(); ++it){
		const QByteArray& subscriptionId = it.key();
		RequestInfo& requestInfo = it.value();

		if (requestInfo.serviceId == serviceId){
			imtgql::CGqlRequest* gqlRequestPtr = dynamic_cast<imtgql::CGqlRequest*>(requestInfo.gqlRequestPtr.GetPtr());
			if (gqlRequestPtr != nullptr && requestInfo.networkRequestPtr != nullptr){
				RegisterSubscription(subscriptionId, *gqlRequestPtr, *requestInfo.networkRequestPtr, errorMessage);
			}

			break;
		}
	}
}


QByteArray CAgentServicesRemoteSubscriberProxyComp::GetServiceId(const QWebSocket& webSocket) const
{
	for (auto it = m_webSocketClientMap.begin(); it != m_webSocketClientMap.end(); ++it){
		if (it.value() == &webSocket){
			return it.key();
		}
	}

	return QByteArray();
}


// public methods of the embedded class NetworkOperation

CAgentServicesRemoteSubscriberProxyComp::NetworkOperation::NetworkOperation(int timeout, const CAgentServicesRemoteSubscriberProxyComp* parent)
{
	Q_ASSERT(parent != nullptr);

	timerFlag = false;

	// If the network reply is finished, the internal event loop will be finished:
	QObject::connect(parent, &CAgentServicesRemoteSubscriberProxyComp::EmitQueryDataReceived, &connectionLoop, &QEventLoop::exit);

	// If the application will be finished, the internal event loop will be also finished:
	QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &connectionLoop, &QEventLoop::quit);

	// If a timeout for the request was defined, start the timer:
	if (timeout > 0){
		timer.setSingleShot(true);

		// If the timer is running out, the internal event loop will be finished:
		QObject::connect(&timer, &QTimer::timeout, &connectionLoop, &QEventLoop::quit);

		timer.start(timeout);
	}
}


CAgentServicesRemoteSubscriberProxyComp::NetworkOperation::~NetworkOperation()
{
	timer.stop();
}


} // namespace agentgql


