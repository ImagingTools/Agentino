// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


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
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);

	QMap<QByteArray, QByteArray> m_remoteSubscriptions; // remoteSubscriptionId -> guiSubscriptionId
};


} // namespace agentinogql
