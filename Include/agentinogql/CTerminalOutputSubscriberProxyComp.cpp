// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTerminalOutputSubscriberProxyComp.h>


// Qt includes
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

// ImtCore includes
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlRequest.h>


namespace agentinogql
{


namespace
{


/**
	Server→agent query path deliberately strips x-authentication-token
	(CSubscriptionManagerComp::SendRequest). Subscription registration did not,
	so the agent tried to validate the central-server user JWT, failed with
	Unauthorized, and never registered OnTerminalOutputChanged. Build a remote
	request that keeps only clientid — same shape as CAgentChangeObserverComp.

	Must construct with RT_SUBSCRIPTION: CGqlRequest::CopyFrom refuses to copy
	when the destination request type differs (default ctor is RT_QUERY), which
	left commandId/params/fields empty and the agent never registered the publisher.
*/
imtgql::CGqlRequest MakeAgentSubscriptionRequest(const imtgql::CGqlRequest& guiRequest)
{
	imtgql::CGqlRequest agentRequest(imtgql::IGqlRequest::RT_SUBSCRIPTION);
	if (!agentRequest.CopyFrom(guiRequest)){
		// Fallback: rebuild essentials so we never send an empty start to the agent.
		agentRequest.SetRequestType(imtgql::IGqlRequest::RT_SUBSCRIPTION);
		agentRequest.SetCommandId(guiRequest.GetCommandId());
		agentRequest.SetParams(guiRequest.GetParams());
		// Fields are required by CGqlPublisherCompBase::IsRequestSupported.
		const imtgql::CGqlFieldObject& fields = guiRequest.GetFields();
		for (const QByteArray& fieldId : fields.GetFieldIds()){
			if (fields.IsObject(fieldId)){
				agentRequest.AddField(fieldId, *fields.GetFieldArgumentObjectPtr(fieldId));
			}
			else{
				agentRequest.AddSimpleField(fieldId);
			}
		}
	}

	const QByteArray agentId = guiRequest.GetHeader(QByteArrayLiteral("clientid"));

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	if (!agentId.isEmpty()){
		headers.insert(QByteArrayLiteral("clientid"), agentId);
	}
	gqlContextPtr->SetHeaders(headers);
	agentRequest.SetGqlContext(gqlContextPtr);

	return agentRequest;
}


} // namespace


// protected methods

// reimplemented (imtgql::IGqlSubscriberController)

bool CTerminalOutputSubscriberProxyComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	const bool retVal = BaseClass::IsRequestSupported(gqlRequest);
	const QByteArray agentId = gqlRequest.GetHeader("clientid");

	return retVal && !agentId.isEmpty();
}


bool CTerminalOutputSubscriberProxyComp::RegisterSubscription(
			const QByteArray& subscriptionId,
			const imtgql::CGqlRequest& gqlRequest,
			const imtrest::IRequest& networkRequest,
			QString& errorMessage)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Internal error");

		return false;
	}

	const bool retVal = BaseClass::RegisterSubscription(subscriptionId, gqlRequest, networkRequest, errorMessage);
	if (!retVal){
		return false;
	}

	const imtgql::CGqlRequest agentRequest = MakeAgentSubscriptionRequest(gqlRequest);
	const QByteArray remoteSubscriptionId = m_subscriptionManagerCompPtr->RegisterSubscription(agentRequest, this);
	if (remoteSubscriptionId.isEmpty()){
		BaseClass::UnregisterSubscription(subscriptionId);
		errorMessage = QStringLiteral("Unable to open terminal output subscription on the agent");
		SendErrorMessage(0, errorMessage, "CTerminalOutputSubscriberProxyComp");

		return false;
	}

	m_remoteSubscriptions.insert(remoteSubscriptionId, subscriptionId);

	return true;
}


bool CTerminalOutputSubscriberProxyComp::UnregisterSubscription(const QByteArray& subscriptionId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return false;
	}

	const bool retVal = BaseClass::UnregisterSubscription(subscriptionId);
	if (retVal){
		for (auto it = m_remoteSubscriptions.constBegin(); it != m_remoteSubscriptions.constEnd(); ++it){
			if (it.value() == subscriptionId){
				m_subscriptionManagerCompPtr->UnregisterSubscription(it.key());
				m_remoteSubscriptions.remove(it.key());
				break;
			}
		}
	}

	return retVal;
}


// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CTerminalOutputSubscriberProxyComp::OnResponseReceived(
			const QByteArray& subscriptionId,
			const QByteArray& subscriptionData)
{
	const QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	const QStringList keys = document.object().keys();
	if (keys.isEmpty()){
		return;
	}

	const QByteArray subscriptionTypeId = keys[0].toUtf8();
	const QByteArray localSubscriptionId = m_remoteSubscriptions.value(subscriptionId);
	if (localSubscriptionId.isEmpty()){
		return;
	}

	const QJsonObject jsonData = document.object().value(QString::fromUtf8(subscriptionTypeId)).toObject();
	const QByteArray body = QJsonDocument(jsonData).toJson(QJsonDocument::Compact);

	// Push only to the GUI subscription that opened this agent subscription — do not
	// PublishData (broadcast) or every open terminal page would receive every session.
	QMutexLocker locker(&m_mutex);

	for (const RequestNetworks& entry : m_registeredSubscribers){
		if (!entry.networkRequests.contains(localSubscriptionId)){
			continue;
		}

		const imtrest::IRequest* networkRequestPtr = entry.networkRequests.value(localSubscriptionId);
		if (networkRequestPtr == nullptr){
			return;
		}

		if (!PushDataToSubscriber(localSubscriptionId, subscriptionTypeId, body, *networkRequestPtr)){
			SendErrorMessage(
						0,
						QString("Unable to relay terminal output to subscription '%1'")
									.arg(QString::fromUtf8(localSubscriptionId)),
						"CTerminalOutputSubscriberProxyComp");
		}

		return;
	}
}


void CTerminalOutputSubscriberProxyComp::OnSubscriptionStatusChanged(
			const QByteArray& /*subscriptionId*/,
			const SubscriptionStatus& /*status*/,
			const QString& /*message*/)
{
}


} // namespace agentinogql
