#include <agentgql/CAgentServicesRemoteSubscriberProxyComp.h>


// Qt includes
#include <QtWebSockets/QWebSocket>

// ImtCore includes
#include <imtgql/CGqlResponse.h>
#include <imtgql/CGqlContext.h>
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
	QByteArray serviceId = gqlRequest.GetHeader("serviceId");
	if (!serviceId.isEmpty()){
		return true;
	}

	return false;
}


bool CAgentServicesRemoteSubscriberProxyComp::RegisterSubscription(
			const QByteArray& subscriptionId,
			const imtgql::CGqlRequest& gqlRequest,
			const imtrest::IRequest& networkRequest,
			QString& errorMessage)
{
	QByteArray serviceId = gqlRequest.GetHeader("serviceId");
	if (!IsRequestSupported(gqlRequest) || !m_connectionCollectionProviderCompPtr.IsValid()){
		return false;
	}

	imtgql::CGqlRequest* gqlRequestPtr = dynamic_cast<imtgql::CGqlRequest*>(gqlRequest.CloneMe());
	if (gqlRequestPtr == nullptr){
		return false;
	}

	imtclientgql::IGqlClient::GqlRequestPtr clientRequestPtr(dynamic_cast<imtgql::IGqlRequest*>(gqlRequestPtr));
	if (!clientRequestPtr.isNull()){
		QByteArray serviceId = gqlRequest.GetHeader("serviceId");
		QByteArray token = gqlRequest.GetHeader("token");
		QUrl url;
		QByteArray serviceTypeName;
		std::shared_ptr<imtservice::IConnectionCollection> connectionCollection = m_connectionCollectionProviderCompPtr->GetConnectionCollection(serviceId);
		if (connectionCollection != nullptr){
			const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollection->GetUrlList());
			const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
			if (objectCollection != nullptr){
				imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
				for (const QByteArray& id: ids){
					const imtservice::IServiceConnectionParam* connectionParamPtr = connectionCollection->GetConnectionMetaInfo(id);
					if (connectionParamPtr == nullptr){
						continue;
					}

					if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
						QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
						QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
						serviceTypeName = connectionCollection->GetServiceTypeName().toUtf8();
						url = connectionParamPtr->GetDefaultUrl();

						imtbase::IObjectCollection::DataPtr dataPtr;
						objectCollection->GetObjectData(id, dataPtr);
						imtservice::CUrlConnectionParam* connectionParam = dynamic_cast<imtservice::CUrlConnectionParam*>(dataPtr.GetPtr());
						if (connectionParam != nullptr){
							url = connectionParam->GetUrl();
						}

						if (url.scheme() == "ws"){
							break;
						}
					}

				}
			}
		}
		if (!m_webSocketClientMap.contains(serviceId)){
			istd::TDelPtr<QWebSocket> webSocketClientPtr(new QWebSocket());
			webSocketClientPtr->open(url);
			NetworkOperation networkOperation(100, this);
			int resultCode = 0;
			for (int i = 0; i < 100; i++){
				networkOperation.timer.start();
				resultCode = networkOperation.connectionLoop.exec(QEventLoop::ExcludeUserInputEvents);
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
				// connect(this, &CAgentServicesRemoteSubscriberProxyComp::EmitWebSocketClose, websocketPtr, &QWebSocket::close);
				connect(websocketPtr, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(OnWebSocketError(QAbstractSocket::SocketError)));


				m_webSocketClientMap.insert(serviceId, websocketPtr);
			}
		}

		if (!m_requestMap.contains(subscriptionId)){
			m_requestMap.insert(subscriptionId, QPair<QByteArray, const imtrest::IRequest*>(serviceId, &networkRequest));
		}

		if (m_webSocketClientMap.contains(serviceId)){
			QByteArray queryData = networkRequest.GetBody();
			m_webSocketClientMap.value(serviceId)->sendTextMessage(queryData);
		}
	}


	return true;
}


bool CAgentServicesRemoteSubscriberProxyComp::UnRegisterSubscription(const QByteArray& subscriptionId)
{
	BaseClass::UnRegisterSubscription(subscriptionId);
	if (m_requestMap.contains(subscriptionId)){
		QByteArray serviceId = m_requestMap.value(subscriptionId).first;

		bool find = false;
		for (QByteArray& subscriptionId : m_requestMap.keys()){
			if (m_requestMap.value(subscriptionId).first == serviceId){
				find = true;

				break;
			}
		}

		if (find == false){
			m_webSocketClientMap.remove(serviceId);
		}
	}
	return true;
}

// reimplemented (imtrest::IRequestEventHandler)
void CAgentServicesRemoteSubscriberProxyComp::OnRequestDestroyed(imtrest::IRequest* request)
{
	for (QByteArray& subscriptionId : m_requestMap.keys()){
		if (m_requestMap.value(subscriptionId).second == request){
			m_requestMap.remove(subscriptionId);

			break;
		}
	}
}


// private slots

void CAgentServicesRemoteSubscriberProxyComp::OnTimeout()
{
	// EnsureWebSocketConnection();
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketConnected()
{
	QWebSocket* webSocketPtr = dynamic_cast<QWebSocket*>(sender());

	if (webSocketPtr == nullptr){
		return;
	}

	QString body = "{ \"type\": \"connection_init\"}";

	webSocketPtr->sendTextMessage(body);
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketDisconnected()
{
	QWebSocket* webSocketPtr = dynamic_cast<QWebSocket*>(sender());

	if (webSocketPtr == nullptr){
		return;
	}

	for (QByteArray& serviceId: m_webSocketClientMap.keys()){
		if (m_webSocketClientMap.value(serviceId) == webSocketPtr){
			for (QByteArray& subscriptionId : m_requestMap.keys()){
				if (m_requestMap.value(subscriptionId).first == serviceId){
					m_requestMap.remove(subscriptionId);
					UnRegisterSubscription(subscriptionId);
				}
			}
			m_webSocketClientMap.remove(serviceId);

			break;
		}
	}

	webSocketPtr->deleteLater();
}


void CAgentServicesRemoteSubscriberProxyComp::OnWebSocketError(QAbstractSocket::SocketError error)
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
	const imtrest::IRequest* networkRequest = m_requestMap.value(subscriptionId).second;

	if (networkRequest == nullptr){
		return;
	}

	QByteArray reponseTypeId = QByteArray("application/json; charset=utf-8");
	const imtrest::IProtocolEngine& engine = networkRequest->GetProtocolEngine();

	imtrest::ConstResponsePtr responsePtr(engine.CreateResponse(*networkRequest,
																imtrest::IProtocolEngine::SC_OPERATION_NOT_AVAILABLE, body, reponseTypeId));
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


