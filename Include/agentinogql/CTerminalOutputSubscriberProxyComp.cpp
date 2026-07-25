// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CTerminalOutputSubscriberProxyComp.h>


// Qt includes
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

// ImtCore includes
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/IGqlContext.h>


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

	// Ownership: the first user to subscribe to a sessionId's output claims it, so a
	// second user who merely learns the sessionId (e.g. from a log) cannot attach to
	// someone else's terminal output. Only checked (read-only) here - the claim itself is
	// recorded after BaseClass::RegisterSubscription actually succeeds, so a rejected
	// registration never leaks a phantom claim that UnregisterSubscription cannot release.
	const QByteArray sessionId = ExtractSessionId(gqlRequest);
	const imtgql::IGqlContext* gqlContextPtr = gqlRequest.GetRequestContext();
	const QByteArray userId = gqlContextPtr != nullptr ? gqlContextPtr->GetUserId() : QByteArray();

	if (!sessionId.isEmpty()){
		QMutexLocker stateLocker(&m_stateMutex);

		const QByteArray existingOwner = m_sessionSubscribers.value(sessionId);
		if (!existingOwner.isEmpty() && existingOwner != userId){
			errorMessage = QStringLiteral("This terminal session belongs to another user");

			return false;
		}
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

	QMutexLocker stateLocker(&m_stateMutex);
	m_remoteSubscriptions.insert(remoteSubscriptionId, subscriptionId);
	if (!sessionId.isEmpty()){
		m_sessionSubscribers.insert(sessionId, userId);
		m_subscriptionSessions.insert(subscriptionId, sessionId);
	}
	SendInfoMessage(
				0,
				QString("[diag] terminal relay registered session=%1 agentSub=%2 guiSub=%3")
							.arg(QString::fromUtf8(sessionId), QString::fromUtf8(remoteSubscriptionId),
									 QString::fromUtf8(subscriptionId)),
				"CTerminalOutputSubscriberProxyComp");

	return true;
}


bool CTerminalOutputSubscriberProxyComp::UnregisterSubscription(const QByteArray& subscriptionId)
{
	if (!m_subscriptionManagerCompPtr.IsValid()){
		return false;
	}

	const bool retVal = BaseClass::UnregisterSubscription(subscriptionId);
	if (retVal){
		QMutexLocker stateLocker(&m_stateMutex);

		for (auto it = m_remoteSubscriptions.constBegin(); it != m_remoteSubscriptions.constEnd(); ++it){
			if (it.value() == subscriptionId){
				const QByteArray remoteSubscriptionId = it.key();
				stateLocker.unlock();
				m_subscriptionManagerCompPtr->UnregisterSubscription(remoteSubscriptionId);
				stateLocker.relock();
				m_remoteSubscriptions.remove(remoteSubscriptionId);
				break;
			}
		}

		const QByteArray sessionId = m_subscriptionSessions.take(subscriptionId);
		if (!sessionId.isEmpty()){
			m_sessionSubscribers.remove(sessionId);
		}
	}

	return retVal;
}


// reimplemented (imtclientgql::IGqlSubscriptionClient)

void CTerminalOutputSubscriberProxyComp::OnResponseReceived(
			const QByteArray& subscriptionId,
			const QByteArray& subscriptionData)
{
	// Relayed synchronously on the subscription manager's delivery thread, exactly like
	// CAgentsSubscriberProxyControllerComp - no owner-thread hop (that dropped every push
	// after the first). m_stateMutex keeps the map read safe against register/unregister.
	const QJsonDocument document = QJsonDocument::fromJson(subscriptionData);
	const QStringList keys = document.object().keys();
	if (keys.isEmpty()){
		return;
	}

	const QByteArray subscriptionTypeId = keys[0].toUtf8();

	QByteArray localSubscriptionId;
	{
		QMutexLocker stateLocker(&m_stateMutex);
		localSubscriptionId = m_remoteSubscriptions.value(subscriptionId);
	}

	// TEMP DIAGNOSTIC: confirms the server received a push from the agent and whether it
	// maps to a live GUI subscription - remove once resolved.
	SendInfoMessage(0, QString("[diag] relay recv type=%1 remoteSub=%2 -> guiSub=%3 (%4 bytes)")
				.arg(QString::fromUtf8(subscriptionTypeId), QString::fromUtf8(subscriptionId),
					localSubscriptionId.isEmpty() ? QStringLiteral("<none>") : QString::fromUtf8(localSubscriptionId))
				.arg(subscriptionData.size()), "CTerminalOutputSubscriberProxyComp");

	if (localSubscriptionId.isEmpty()){
		return;
	}

	const QJsonObject jsonData = document.object().value(QString::fromUtf8(subscriptionTypeId)).toObject();
	const QByteArray body = QJsonDocument(jsonData).toJson(QJsonDocument::Compact);

	// Push only to the GUI subscription that opened this agent subscription — do not
	// PublishData (broadcast) or every open terminal page would receive every session.
	// The push runs while holding the base m_mutex so the stored networkRequest cannot be
	// freed by a concurrent socket disconnect mid-push (see CGqlPublisherCompBase).
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
	// The subscription manager only ever reports SS_REGISTERED here (never a loss), so
	// there is nothing actionable to relay - matching CAgentsSubscriberProxyControllerComp.
	// Detecting a genuine server<->agent drop needs the agent-connection status path
	// (as in CAgentChangeObserverComp), not this callback.
}


QByteArray CTerminalOutputSubscriberProxyComp::ExtractSessionId(const imtgql::CGqlRequest& gqlRequest)
{
	const imtgql::CGqlParamObject* inputPtr = gqlRequest.GetParamObject("input");
	if (inputPtr == nullptr){
		return QByteArray();
	}

	return inputPtr->GetParamArgumentValue("sessionId").toByteArray();
}


} // namespace agentinogql
