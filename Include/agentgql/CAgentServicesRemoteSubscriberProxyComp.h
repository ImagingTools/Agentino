#pragma once


// Qt includes
#include <QtWebSockets/QWebSocket>
#include <QtCore/QTimer>

// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtservergql/CGqlPublisherCompBase.h>



namespace agentgql
{


class CAgentServicesRemoteSubscriberProxyComp: public QObject, public imtservergql::CGqlPublisherCompBase
{
	Q_OBJECT
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

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
	virtual bool UnregisterSubscription(const QByteArray& subscriptionId) override;

	// reimplemented (imtrest::IRequestEventHandler)
	virtual void OnRequestDestroyed(imtrest::IRequest* request) override;

Q_SIGNALS:
	void EmitQueryDataReceived(int resultCode = 1);

private Q_SLOTS:
	void OnWebSocketConnected();
	void OnWebSocketDisconnected();
	void OnWebSocketError(QAbstractSocket::SocketError error);
	void OnWebSocketTextMessageReceived(const QString& message);
	void OnWebSocketBinaryMessageReceived(const QByteArray& message);

private:
	void TryReconnect(const QByteArray& serviceId);
	QByteArray GetServiceId(const QWebSocket& webSocket) const;
	
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
	
	struct RequestInfo
	{
		QByteArray serviceId;
		const imtrest::IRequest* networkRequestPtr;
		imtclientgql::IGqlClient::GqlRequestPtr gqlRequestPtr;
	};

private:
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);

	QMap<QByteArray, QWebSocket*> m_webSocketClientMap;
	QMap<QByteArray, RequestInfo> m_requestMap;
	QMap<QByteArray, QTimer*> m_reconnectTimers;
};


} // namespace agentgql


