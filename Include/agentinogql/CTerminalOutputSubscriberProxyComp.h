// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMutex>

// ImtCore includes
#include <imtclientgql/IGqlClient.h>
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtservergql/CGqlPublisherCompBase.h>


namespace agentinogql
{


/**
	Server-side bridge for \c OnTerminalOutputChanged.

	Same lifecycle as \ref CAgentsSubscriberProxyControllerComp (open a per-agent GQL
	subscription when a GUI client subscribes with a \c clientid header, tear it down on
	unregister), but relays each agent push to the single matching GUI subscription
	instead of broadcasting — terminal output is session-scoped and must not fan out to
	every open terminal page.

	Like that sibling, \ref OnResponseReceived relays synchronously on whatever thread the
	subscription manager delivers the agent push on. The only shared state
	(\ref m_remoteSubscriptions and the ownership maps) is therefore guarded by
	\ref m_stateMutex, which is enough to make register/unregister (GQL worker threads)
	safe against relay (WebSocket I/O thread) without depending on this component having a
	running Qt event loop of its own — an earlier owner-thread-marshalling attempt silently
	dropped every push after the first because the queued calls landed on a thread whose
	event loop was not the one processing them.
*/
class CTerminalOutputSubscriberProxyComp:
			public imtservergql::CGqlPublisherCompBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CTerminalOutputSubscriberProxyComp);
		I_REGISTER_INTERFACE(imtclientgql::IGqlSubscriptionClient);
		I_ASSIGN(m_subscriptionManagerCompPtr, "SubscriptionManager", "Subscription agent manager", true, "SubscriptionManager");
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

	// reimplemented (imtclientgql::IGqlSubscriptionClient)
	virtual void OnResponseReceived(const QByteArray& subscriptionId, const QByteArray& subscriptionData) override;
	virtual void OnSubscriptionStatusChanged(
				const QByteArray& subscriptionId,
				const SubscriptionStatus& status,
				const QString& message) override;

private:
	static QByteArray ExtractSessionId(const imtgql::CGqlRequest& gqlRequest);

private:
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);

	// Guards every member below. Never held across a call into the base class or into
	// m_subscriptionManagerCompPtr, so it cannot deadlock against the base's m_mutex
	// (which independently guards m_registeredSubscribers).
	QMutex m_stateMutex;

	QMap<QByteArray, QByteArray> m_remoteSubscriptions; // remoteSubscriptionId -> guiSubscriptionId

	// sessionId this GUI subscription was registered for, recorded at RegisterSubscription
	// time so UnregisterSubscription can always find and release the matching
	// m_sessionSubscribers entry.
	QMap<QByteArray, QByteArray> m_subscriptionSessions; // guiSubscriptionId -> sessionId

	// Claims one sessionId's output stream for the user who first subscribes to it, so a
	// second user who merely learns the sessionId cannot attach to someone else's output.
	QMap<QByteArray, QByteArray> m_sessionSubscribers; // sessionId -> owning userId
};


} // namespace agentinogql
