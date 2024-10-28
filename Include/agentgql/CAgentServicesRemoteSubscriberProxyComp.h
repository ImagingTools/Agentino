#pragma once

// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtgql/CGqlSubscriberControllerCompBase.h>

// Qt includes
#include <QtWebSockets/QWebSocket>
#include <QtCore/QTimer>


namespace agentgql
{


class CAgentServicesRemoteSubscriberProxyComp: public QObject, public imtgql::CGqlSubscriberControllerCompBase //, virtual public imtrest::IRequestServlet
{
	Q_OBJECT
public:
	typedef imtgql::CGqlSubscriberControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentServicesRemoteSubscriberProxyComp)
		I_ASSIGN(m_connectionCollectionProviderCompPtr, "ConnectionCollectionProvider", "Application info", true, "ConnectionCollectionProvider");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlSubscriberController)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
	virtual bool RegisterSubscription(
				const QByteArray& subscriptionId,
				const imtgql::CGqlRequest& gqlRequest,
				const imtrest::IRequest& networkRequest,
				QString& errorMessage) override;
	virtual bool UnRegisterSubscription(const QByteArray& subscriptionId) override;

	// reimplemented (imtrest::IRequestEventHandler)
	virtual void OnRequestDestroyed(imtrest::IRequest* request) override;

Q_SIGNALS:
	void EmitQueryDataReceived(int resultCode = 1);

private Q_SLOTS:
	void OnTimeout();
	void OnWebSocketConnected();
	void OnWebSocketDisconnected();
	void OnWebSocketError(QAbstractSocket::SocketError error);
	void OnWebSocketTextMessageReceived(const QString& message);
	void OnWebSocketBinaryMessageReceived(const QByteArray& message);

private:
	class NetworkOperation
	{
	public:
		NetworkOperation() = delete;
		NetworkOperation(int timeout, const CAgentServicesRemoteSubscriberProxyComp* parent);
		~NetworkOperation();

		QEventLoop connectionLoop;
		bool timerFlag;
		QTimer timer;
	};

private:
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);

	QMap<QByteArray, QWebSocket*> m_webSocketClientMap; // <serviceId, WebSocketClient<
	QMap<QByteArray,  QPair<QByteArray, const imtrest::IRequest*>> m_requestMap;  // <subscriptionId, QPair(serviceId, networkRequest pointer)>
};


} // namespace agentgql


