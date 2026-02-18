// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtclientgql/IGqlSubscriptionManager.h>
#include <imtclientgql/IGqlClient.h>
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlPublisherCompBase.h>


namespace agentinogql
{


class CAgentsSubscriberProxyControllerComp:
			public imtservergql::CGqlPublisherCompBase,
			virtual public imtclientgql::IGqlSubscriptionClient
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentsSubscriberProxyControllerComp);
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
	virtual void OnSubscriptionStatusChanged(const QByteArray& subscriptionId, const SubscriptionStatus& status, const QString& message) override;

private:
	I_REF(imtclientgql::IGqlSubscriptionManager, m_subscriptionManagerCompPtr);
	
private:
	QMap<QByteArray, QByteArray> m_remoteSubscriptions; // <remoteSubscriptionId, subscriptionId>
};


} // namespace agentinogql


