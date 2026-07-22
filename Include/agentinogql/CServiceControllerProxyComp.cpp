// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceControllerProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

// ACF includes
#include <istd/CChangeGroup.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/ServiceEndpointId.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>

// ImtCore includes
#include <imtbase/imtbase.h>
#include <imtcom/IServerConnectionInterface.h>
#include <imtgql/CGqlContext.h>
#include <imtclientgql/IAsyncGqlRequestToken.h>


namespace agentinogql
{


// public methods

CServiceControllerProxyComp::CServiceControllerProxyComp()
{
}


// reimplemented (IServiceCollectionSynchronizer)

bool CServiceControllerProxyComp::ApplyServiceDataToMirror(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			const sdl::V1_0::agentino::CServiceData& serviceData,
			QString& errorMessage)
{
	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		return false;
	}

	istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation. Error: %1").arg(errorMessage);
		return false;
	}

	serviceInfoPtr->SetObjectUuid(serviceId);

	const QString serviceName = serviceInfoPtr->GetServiceName();
	const QString serviceDescription = serviceInfoPtr->GetServiceDescription();

	// The agent is the single source of truth for its own service's live status — refresh it
	// from every GetService response, not just on first insert into the mirror. Without this,
	// a service that stays Running across an agent disconnect/reconnect cycle never leaves the
	// UNDEFINED status the disconnect handler set: the agent has no status *change* to push
	// (OnAgentServiceStatusChanged), since from its own perspective nothing happened.
	agentinodata::IServiceStatusInfo::ServiceStatus status =
				agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
	if (serviceData.status.has_value()){
		agentinodata::GetServiceStatusFromRepresentation(*serviceData.status, status);
	}
	SetServiceStatus(serviceId, status);

	if (m_serviceManagerCompPtr->ServiceExists(agentId, serviceId)){
		if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr, serviceName, serviceDescription, false)){
			errorMessage = QString("Unable to update service '%1' in the server mirror").arg(qPrintable(serviceId));
			return false;
		}
	}
	else{
		if (!m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr, serviceId, serviceName, serviceDescription)){
			errorMessage = QString("Unable to add service '%1' to the server mirror").arg(qPrintable(serviceId));
			return false;
		}
	}

	return true;
}


namespace
{


bool BuildGetServiceRequestForAgent(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			imtgql::CGqlRequest& getGqlRequest,
			QString& errorMessage)
{
	// Machine-to-agent poll: address the agent via clientid only.
	// Do NOT attach a user JWT — deferred sync often runs without a request context, and a
	// stale/invalid token yields "Unauthorized" on the agent while empty token is accepted.
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert(QByteArrayLiteral("clientid"), agentId);
	gqlContextPtr->SetHeaders(headers);
	getGqlRequest.SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::GetServiceRequestArguments getArguments;
	getArguments.input.emplace();
	getArguments.input->id = serviceId;

	if (!sdl::V1_0::agentino::CGetServiceGqlRequest::SetupGqlRequest(getGqlRequest, getArguments)){
		errorMessage = QString("Unable to build 'GetService' request for '%1'").arg(qPrintable(serviceId));
		return false;
	}

	return true;
}


} // namespace


bool CServiceControllerProxyComp::StartAsyncServiceSync(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			QString& errorMessage)
{
	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		return false;
	}

	if (m_enrollmentControllerCompPtr.IsValid() && !agentId.isEmpty()){
		const EnrollmentRecord record = m_enrollmentControllerCompPtr->Get(agentId);
		if (record.status != EnrollmentStatus::Approved){
			errorMessage = QStringLiteral("Agent '%1' is not Approved; service mirror sync denied")
						.arg(QString::fromUtf8(agentId));
			return false;
		}
	}

	if (!HasAsyncApiClient()){
		errorMessage = QStringLiteral("AsyncApiClient not configured; cannot non-blockingly sync service");
		return false;
	}

	imtgql::CGqlRequest getGqlRequest;
	if (!BuildGetServiceRequestForAgent(agentId, serviceId, getGqlRequest, errorMessage)){
		return false;
	}
	AppendServiceDataFields(getGqlRequest);

	const QByteArray agentIdCopy = agentId;
	const QByteArray serviceIdCopy = serviceId;
	imtclientgql::IAsyncGqlRequestTokenPtr token =
				SendModelRequestAsync<sdl::V1_0::agentino::CServiceData>(
							getGqlRequest,
							[this, agentIdCopy, serviceIdCopy](
										sdl::V1_0::agentino::CServiceData serviceData,
										QString fetchError) {
								// This completion runs on the SubscriptionManager thread, not this
								// component's own thread - the same issue as
								// StartAsyncAgentReconcile's ServicesList completion (see the
								// comment there). ApplyServicesListReconcile fires one of these
								// GetService requests per service in a tight loop, so several of
								// these completions can race each other and the reconcile's own
								// seed step across threads. ApplyServiceDataToMirror's
								// ServiceExists()-then-AddService()/SetService() is a classic
								// check-then-act - unprotected, two racing calls for the SAME
								// service can both see "does not exist" and both insert, leaving a
								// duplicate row in the mirror (reproduced: reconciling an agent
								// with services open left one of them duplicated in the mirror,
								// and the duplicate's presence is exactly what then also broke its
								// own status relay). Hop before touching the mirror, same as
								// everywhere else in this file that mutates it from a callback.
								QMetaObject::invokeMethod(
											this,
											[this, agentIdCopy, serviceIdCopy, serviceData, fetchError]() mutable {
												const bool hasId = serviceData.id.has_value() && !serviceData.id->isEmpty();
												const bool hasName = serviceData.name.has_value() && !serviceData.name->isEmpty();
												if (!fetchError.isEmpty()){
													SendErrorMessage(
																0,
																QStringLiteral(
																			"Async GetService for mirror failed (agent '%1' service '%2'): %3")
																			.arg(QString::fromUtf8(agentIdCopy),
																				 QString::fromUtf8(serviceIdCopy),
																				 fetchError),
																"CServiceControllerProxyComp");
													// Soft error with a usable body: still apply (seed/list may be thin).
													if (!hasId && !hasName){
														return;
													}
												}
												if (!hasId){
													serviceData.id = serviceIdCopy;
												}
												if (!hasName){
													serviceData.name = QString::fromUtf8(serviceIdCopy);
												}

												QString applyError;
												if (!ApplyServiceDataToMirror(agentIdCopy, serviceIdCopy, serviceData, applyError)){
													SendErrorMessage(
																0,
																applyError,
																"CServiceControllerProxyComp");
												}
											},
											Qt::QueuedConnection);
							});

	if (!token.IsValid()){
		errorMessage = QStringLiteral("AsyncApiClient failed to dispatch GetService");
		return false;
	}

	return true;
}


bool CServiceControllerProxyComp::SyncServiceInMirror(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			QString& errorMessage)
{
	// Async only: blocking Wait on the WebSocket / GQL-worker stack deadlocks
	// (nested query_data is not drained while m_isProcessingMessage is set).
	return StartAsyncServiceSync(agentId, serviceId, errorMessage);
}


bool CServiceControllerProxyComp::RemoveServicesInMirror(
			const QByteArray& agentId,
			const QByteArrayList& serviceIds,
			QString& errorMessage)
{
	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		errorMessage = QString("Unable to remove service(s) '%1' from the server mirror").arg(qPrintable(serviceIds.join(';')));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	RemoveServiceStatuses(serviceIds);

	return true;
}


bool CServiceControllerProxyComp::SyncAgentServicesInMirror(
			const QByteArray& agentId,
			QString& errorMessage)
{
	if (m_agentsBeingReconciled.contains(agentId)){
		// Nested call from SendModelRequest local event loop: queue a follow-up pass
		// instead of dropping the reconcile (Architecture P3/P6).
		QueueReconcile(agentId);
		return true;
	}

	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	// Live/reconcile path enrollment gate (revoke must stop mirror ingestion immediately).
	if (m_enrollmentControllerCompPtr.IsValid() && !agentId.isEmpty()){
		const EnrollmentRecord record = m_enrollmentControllerCompPtr->Get(agentId);
		if (record.status != EnrollmentStatus::Approved){
			errorMessage = QStringLiteral("Agent '%1' is not Approved; mirror reconcile denied")
						.arg(QString::fromUtf8(agentId));
			return false;
		}
	}

	// Reconcile is always non-blocking via TAsyncClientRequestManagerCompWrap
	// (AsyncApiClient → SubscriptionManager IAsyncGqlClient).
	return StartAsyncAgentReconcile(agentId, errorMessage);
}


bool CServiceControllerProxyComp::StartAsyncAgentReconcile(
			const QByteArray& agentId,
			QString& errorMessage)
{
	m_agentsBeingReconciled.insert(agentId);

	imtgql::CGqlRequest listGqlRequest;
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	listGqlRequest.SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::ServicesListRequestArguments listArguments;
	listArguments.input.emplace();
	sdl::V1_0::agentino::ServicesListRequestInfo listInfo;
	listInfo.items.isIdRequested = true;

	if (!sdl::V1_0::agentino::CServicesListGqlRequest::SetupGqlRequest(listGqlRequest, listArguments, listInfo)){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QString("Unable to build 'ServicesList' request for agent '%1'").arg(qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return false;
	}
	AppendServicesListFields(listGqlRequest);

	// Capture agentId; callback runs on SubscriptionManager thread without nested loop.
	const QByteArray agentIdCopy = agentId;
	CServiceControllerProxyComp* self = const_cast<CServiceControllerProxyComp*>(this);
	imtclientgql::IAsyncGqlRequestTokenPtr token = SendModelRequestAsync<sdl::V1_0::agentino::CServiceListPayload>(
				listGqlRequest,
				[self, agentIdCopy](sdl::V1_0::agentino::CServiceListPayload listPayload, QString listError) {
					// This completion runs on the SubscriptionManager thread, not this
					// component's own thread (see the old comment below, kept for context).
					// ApplyServicesListReconcile mutates m_agentsBeingReconciled and writes
					// ServiceManager / ServiceStatusCollection, whose change notifications
					// cascade synchronously to observers (including AgentChangeObserver's
					// relay to GUI subscribers) on whatever thread calls SetObjectData.
					// Left unhopped, this raced with everything else that touches those same
					// collections on the main thread and intermittently dropped/corrupted the
					// reconcile for one of several services processed in the same batch -
					// QueueServiceSync/QueueReconcile just below already hop for the same
					// reason; this completion callback was the one path that didn't.
					QMetaObject::invokeMethod(
								self,
								[self, agentIdCopy, listPayload, listError]() {
									// Soft GraphQL / parse errors may still carry usable items (OptRead).
									// Only abort when the items field is missing (incomplete body).
									// Empty array items:[] is a valid "agent has no services" result.
									const bool hasItemsField = listPayload.items.has_value();
									if (!listError.isEmpty()){
										self->SendErrorMessage(0, listError, "CServiceControllerProxyComp");
										if (!hasItemsField){
											self->m_agentsBeingReconciled.remove(agentIdCopy);
											// Do NOT touch m_pendingReconcile here — that field is only ever
											// safe to read/write via the QueueReconcile()/OnDeferredReconcile()
											// hop, kept separate from this one for clarity. If a nested
											// reconcile was requested while this one was in flight, that
											// QueueReconcile() call already scheduled OnDeferredReconcile(),
											// which will drain it now that m_agentsBeingReconciled is clear.
											return;
										}
									}
									QString applyError;
									self->ApplyServicesListReconcile(agentIdCopy, listPayload, applyError);
								},
								Qt::QueuedConnection);
				});

	if (!token.IsValid()){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QStringLiteral("AsyncApiClient failed to dispatch ServicesList");
		return false;
	}

	errorMessage.clear();
	return true; // scheduled
}


bool CServiceControllerProxyComp::ApplyServicesListReconcile(
			const QByteArray& agentId,
			const sdl::V1_0::agentino::CServiceListPayload& listPayload,
			QString& errorMessage)
{
	// Missing items key → incomplete parse / wrong body. Never wipe the mirror.
	if (!listPayload.items.has_value()){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QStringLiteral(
					"ServicesList for agent '%1' returned no items field; mirror left unchanged")
								.arg(QString::fromUtf8(agentId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	QByteArrayList agentServiceIds;
	int rawItemCount = 0;
	// Keep list rows so we can seed the mirror before async GetService returns.
	QList<sdl::V1_0::agentino::CServiceItem> listItems;
	rawItemCount = listPayload.items->size();
	for (const istd::TNullableValue<sdl::V1_0::agentino::CServiceItem>& item : *listPayload.items){
		if (item.has_value() && item->id.has_value() && !item->id->isEmpty()){
			agentServiceIds << *item->id;
			listItems << *item;
		}
	}

	// Protocol safety: items present but no ids → broken selection set / parse. Never wipe mirror.
	if (rawItemCount > 0 && agentServiceIds.isEmpty()){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QString(
					"ServicesList for agent '%1' returned %2 item(s) without ids; mirror left unchanged")
								.arg(qPrintable(agentId))
								.arg(rawItemCount);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	const QByteArrayList mirrorIds = GetMirrorServiceIds(agentId);

	QByteArrayList staleIds;
	for (const QByteArray& mirrorId : mirrorIds){
		if (!agentServiceIds.contains(mirrorId)){
			staleIds << mirrorId;
		}
	}

	// Temporary diagnostic: shows exactly what the agent's ServicesList response contained
	// and what the reconcile decided to do with it - confirms whether a service that never
	// gets its status corrected on reconnect is (a) missing from the agent's own response,
	// (b) present but wrongly classified as stale/removed, or (c) present and processed
	// normally (in which case the drop happens further down/elsewhere). Remove once closed.
	SendInfoMessage(
				0,
				QString("ApplyServicesListReconcile for agent '%1': rawItemCount=%2 agentServiceIds=[%3] mirrorIds=[%4] staleIds=[%5]")
							.arg(QString::fromUtf8(agentId))
							.arg(rawItemCount)
							.arg(QString::fromUtf8(agentServiceIds.join(", ")))
							.arg(QString::fromUtf8(mirrorIds.join(", ")))
							.arg(QString::fromUtf8(staleIds.join(", "))),
				"CServiceControllerProxyComp");

	// Batch CF_SERVICE_* / status notifications into one GUI poke.
	istd::CChangeGroup serviceChangeGroup(m_serviceManagerCompPtr.GetPtr());
	istd::CChangeGroup statusChangeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	QStringList aggregatedErrors;

	if (!staleIds.isEmpty()){
		QString removeError;
		if (!RemoveServicesInMirror(agentId, staleIds, removeError)){
			aggregatedErrors << removeError;
		}
	}

	// Seed the mirror immediately from ServicesList rows so Agents.services and Topology
	// update without waiting for per-service GetService (which can lag or soft-fail).
	// GetService still runs after this to fill connections / full descriptor.
	for (const sdl::V1_0::agentino::CServiceItem& item : listItems){
		const QByteArray serviceId = *item.id;
		sdl::V1_0::agentino::CServiceData seed;
		seed.id = serviceId;
		if (item.name.has_value()){
			seed.name = *item.name;
		}
		else{
			// AddService/InsertNewObject need a non-empty display name.
			seed.name = QString::fromUtf8(serviceId);
		}
		if (item.description.has_value()){
			seed.description = *item.description;
		}
		if (item.path.has_value()){
			seed.path = *item.path;
		}
		if (item.version.has_value()){
			seed.version = *item.version;
		}
		if (item.status.has_value()){
			seed.status = *item.status;
		}
		if (item.typeId.has_value() && !item.typeId->isEmpty()){
			seed.serviceTypeId = QString::fromUtf8(*item.typeId);
		}

		QString seedError;
		if (!ApplyServiceDataToMirror(agentId, serviceId, seed, seedError)){
			aggregatedErrors << seedError;
		}
	}

	for (const QByteArray& serviceId : agentServiceIds){
		// Non-blocking GetService per service (SyncServiceInMirror → StartAsyncServiceSync
		// when AsyncApiClient is set). Never Wait on the completion path of ServicesList.
		QString syncError;
		if (!SyncServiceInMirror(agentId, serviceId, syncError)){
			aggregatedErrors << syncError;
			// Continue: one broken service must not abort the full reconcile.
		}
	}

	// Reconcile "scheduled" (list applied + per-service GetService fired). Mirror
	// already has seed rows; GetService upgrades them when it completes.
	m_agentsBeingReconciled.remove(agentId);

	// Do NOT touch m_pendingReconcile here — ApplyServicesListReconcile runs on the
	// SubscriptionManager thread (it is the async ServicesList completion callback), not
	// this component's thread, and m_pendingReconcile is only ever safe to read/write via
	// the QueueReconcile()/OnDeferredReconcile() queued hop. Concurrent unsynchronized
	// access from here corrupted the QSet and crashed OnDeferredReconcile()'s iteration.
	// If a nested reconcile was requested while this one was in flight, that QueueReconcile()
	// call already scheduled OnDeferredReconcile(), which will drain it now that
	// m_agentsBeingReconciled is clear (just above).

	if (!aggregatedErrors.isEmpty()){
		errorMessage = aggregatedErrors.join("; ");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	errorMessage.clear();

	return true;
}


void CServiceControllerProxyComp::QueueReconcile(const QByteArray& agentId) const
{
	if (agentId.isEmpty()){
		return;
	}

	// Hop to this component's thread. Callers (notify/reconcile) often run on a GQL
	// worker; never touch QTimer / QObject from that thread.
	CServiceControllerProxyComp* self = const_cast<CServiceControllerProxyComp*>(this);
	const bool queued = QMetaObject::invokeMethod(
				self,
				[self, agentId]() {
					self->m_pendingReconcile.insert(agentId);
					// Second hop so we leave any nested event loop before reconcile.
					QMetaObject::invokeMethod(
								self,
								&CServiceControllerProxyComp::OnDeferredReconcile,
								Qt::QueuedConnection);
				},
				Qt::QueuedConnection);

	if (!queued){
		SendErrorMessage(
					0,
					QStringLiteral("Unable to queue agent reconcile for '%1'")
								.arg(QString::fromUtf8(agentId)),
					"CServiceControllerProxyComp");
	}
}


void CServiceControllerProxyComp::QueueServiceSync(
			const QByteArray& agentId,
			const QByteArray& serviceId) const
{
	if (agentId.isEmpty() || serviceId.isEmpty()){
		return;
	}

	// Hop to this component's thread, then dispatch non-blocking GetService.
	// Never block here with Sync Wait — that freezes the server and starves agent workers.
	CServiceControllerProxyComp* self = const_cast<CServiceControllerProxyComp*>(this);
	const bool queued = QMetaObject::invokeMethod(
				self,
				[self, agentId, serviceId]() {
					QString err;
					if (!self->StartAsyncServiceSync(agentId, serviceId, err)){
						self->SendErrorMessage(
									0,
									QStringLiteral(
												"Unable to start async live service sync for agent '%1' service '%2'%3")
												.arg(QString::fromUtf8(agentId),
													 QString::fromUtf8(serviceId),
													 err.isEmpty()
																 ? QString()
																 : QStringLiteral(": %1").arg(err)),
									"CServiceControllerProxyComp");
					}
				},
				Qt::QueuedConnection);

	if (!queued){
		SendErrorMessage(
					0,
					QStringLiteral("Unable to queue live service sync for agent '%1' service '%2'")
								.arg(QString::fromUtf8(agentId), QString::fromUtf8(serviceId)),
					"CServiceControllerProxyComp");
	}
}


void CServiceControllerProxyComp::OnDeferredReconcile()
{
	const QList<QByteArray> agents = m_pendingReconcile.values();
	m_pendingReconcile.clear();
	for (const QByteArray& agentId : agents){
		QString err;
		SyncAgentServicesInMirror(agentId, err);
	}
}




// protected methods

sdl::V1_0::agentino::CServiceData CServiceControllerProxyComp::OnGetService(
			const sdl::V1_0::agentino::CGetServiceGqlRequest& getServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceData();
	}

	sdl::V1_0::agentino::GetServiceRequestArguments arguments = getServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceData();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	if (inputParamPtr == nullptr){
		return sdl::V1_0::agentino::CServiceData();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	sdl::V1_0::agentino::CServiceData response = SendModelRequest<sdl::V1_0::agentino::CServiceData>(gqlRequest, errorMessage);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	// Agent is the authoritative source of truth. When the forward failed (agent offline / no
	// longer has this service), fall back to the last-known server mirror representation so the
	// item stays viewable/removable instead of becoming permanently stuck.
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		errorMessage.clear();

		istd::TDelPtr<agentinodata::CServiceInfo> mirrorInfoPtr;
		mirrorInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
		if (!mirrorInfoPtr.IsValid()
					|| !agentinodata::GetRepresentationFromService(response, *mirrorInfoPtr.GetPtr())){
			errorMessage = QString("Unable to get service '%1'. Error: unavailable on the agent").arg(qPrintable(serviceId));
			SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

			return sdl::V1_0::agentino::CServiceData();
		}

		response.id = serviceId;

		return response;
	}

	// The agent answered authoritatively; the server neither reads-requires nor writes its
	// mirror here. The candidate pick-list is no longer packed into GetService — the editor
	// asks for it lazily via AvailableConnections. Each output connection already carries its
	// stored dependantConnectionId + cached connectionParam from the agent's descriptor.
	return response;
}


sdl::V1_0::imtbase::CUpdatedNotificationPayload CServiceControllerProxyComp::OnUpdateService(
			const sdl::V1_0::agentino::CUpdateServiceGqlRequest& updateServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	sdl::V1_0::agentino::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::agentino::CServiceData serviceData;
	if (arguments.input->item){
		serviceData = *arguments.input->item;
	}

	sdl::V1_0::imtbase::CUpdatedNotificationPayload retVal =
			SendModelRequest<sdl::V1_0::imtbase::CUpdatedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		errorMessage = DescribeProxyError(serviceId, errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	// Best-effort cross-agent URL propagation: if the previous state is already mirrored,
	// diff its connection URLs against the new input and push changes to dependent consumers.
	// The agent forward above already succeeded, so a missing mirror row (reactive sync not yet
	// applied) must NOT fail the update — just skip propagation.
	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (serviceInfoPtr.IsValid()){
		sdl::V1_0::agentino::CServiceData currentServiceData;
		if (agentinodata::GetRepresentationFromService(currentServiceData, *serviceInfoPtr.GetPtr())){
			const QList<ChangedConnectionUrl> changedConnections = GetChangedConnectionUrl(currentServiceData, serviceData);

			for (const ChangedConnectionUrl& changed : changedConnections){
				// Consumers reference this endpoint as "<producerServiceId>|<addressableId>" -
				// addressableId is either the main connection's own id or one of its extern
				// entries' uuid (see GetChangedConnectionUrl).
				const QByteArray endpointId =
							agentinodata::ServiceEndpointId::Make(serviceId, changed.addressableId);

				sdl::V1_0::imtbase::CServerConnectionParam newConnectionParam;
				newConnectionParam.host = changed.host;
				newConnectionParam.isSecure = changed.isSecure;
				newConnectionParam.httpPort = changed.httpPort;
				newConnectionParam.httpPath = changed.httpPath;
				newConnectionParam.wsPort = changed.wsPort;

				// Each consumer must be told on *its own* agent — the producer's agent
				// cannot update a service that lives somewhere else. The consumer's agent
				// addresses the link by the consumer's local slot id, not by our endpoint id.
				const QVector<ConsumerRef> consumers = FindConsumersOfEndpoint(endpointId);
				for (const ConsumerRef& consumer : consumers){
					UpdateConnectionForService(
								consumer.serviceId,
								consumer.agentId,
								consumer.slot,
								newConnectionParam);
				}
			}
		}
	}

	// Architecture: the update is applied on the owning Agent only (single source of truth).
	// The server does NOT write the current service into its mirror here — that happens
	// reactively when the Agent pushes NotifyAgentServicesCollectionChanged('updated') ->
	// HandleAgentServiceCollectionNotify -> SyncServiceInMirror. Only the cross-agent
	// dependent-connection URL propagation above stays synchronous so consumers on other agents
	// pick up a changed producer URL immediately.
	return retVal;
}


sdl::V1_0::imtbase::CAddedNotificationPayload CServiceControllerProxyComp::OnAddService(
			const sdl::V1_0::agentino::CAddServiceGqlRequest& addServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	sdl::V1_0::agentino::AddServiceRequestArguments arguments = addServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::imtbase::CAddedNotificationPayload retVal = SendModelRequest<sdl::V1_0::imtbase::CAddedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	// Eager mirror write from the mutation payload so Topology / Agents.Services update
	// immediately. Relying only on Notify+async GetService left the mirror empty when the
	// notify path lagged or failed — empty Services column and missing topology nodes.
	// Agent remains SSOT for live data; QueueServiceSync still refreshes from GetService.
	if (!agentId.isEmpty() && !serviceId.isEmpty() && arguments.input->item.has_value()){
		sdl::V1_0::agentino::CServiceData serviceData = *arguments.input->item;
		// Freshly created services are Stopped until Start; never seed UNDEFINED.
		if (!serviceData.status.has_value()
					|| serviceData.status->trimmed().isEmpty()
					|| serviceData.status->compare(QStringLiteral("undefined"), Qt::CaseInsensitive) == 0
					|| serviceData.status->compare(QStringLiteral("UNDEFINED"), Qt::CaseInsensitive) == 0){
			serviceData.status = QStringLiteral("notRunning");
		}
		if (!serviceData.id.has_value() || serviceData.id->isEmpty()){
			serviceData.id = serviceId;
		}

		QString mirrorError;
		if (!const_cast<CServiceControllerProxyComp*>(this)->ApplyServiceDataToMirror(
					agentId, serviceId, serviceData, mirrorError)){
			SendErrorMessage(
						0,
						QStringLiteral("AddService: eager mirror write failed for '%1': %2")
									.arg(QString::fromUtf8(serviceId), mirrorError),
						"CServiceControllerProxyComp");
		}

		// Best-effort full refresh from the agent (connections etc.) without blocking.
		QueueServiceSync(agentId, serviceId);
	}

	return retVal;
}


// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnStartService(
			const sdl::V1_0::agentino::CStartServiceGqlRequest& startServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::StartServiceRequestArguments arguments = startServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	// Start is executed strictly on the owning Agent (single source of truth). The server does
	// NOT write service status here — the Agent's supervisor emits the real status transitions
	// (Starting -> Running / Failed), which arrive via OnAgentServiceStatusChanged and are the
	// only writer of the server status projection. The returned status is a transient ack only.
	// Async only: return a transient STARTING ack; real status arrives via agent subscription.
	sdl::V1_0::agentino::CServiceStatusResponse retVal;
	retVal.status = sdl::V1_0::agentino::ServiceStatus::STARTING;

	imtclientgql::IAsyncGqlRequestTokenPtr token = SendModelRequestAsync<sdl::V1_0::agentino::CServiceStatusResponse>(
				gqlRequest,
				[this](sdl::V1_0::agentino::CServiceStatusResponse response, QString err) {
					Q_UNUSED(response);
					if (!err.isEmpty()){
						SendErrorMessage(0, err, "CServiceControllerProxyComp");
					}
				});
	if (!token.IsValid()){
		errorMessage = QStringLiteral("AsyncApiClient failed to dispatch StartService");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	errorMessage.clear();
	return retVal;
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnStopService(
			const sdl::V1_0::agentino::CStopServiceGqlRequest& stopServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::StopServiceRequestArguments arguments = stopServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	// Stop is executed strictly on the owning Agent (single source of truth). The server does NOT
	// write service status here — the Agent's supervisor emits the real transitions (Stopping ->
	// Stopped) via OnAgentServiceStatusChanged, the only writer of the server status projection.
	// The returned status is a transient ack only.
	// Async only: return a transient STOPPING ack; real status arrives via agent subscription.
	sdl::V1_0::agentino::CServiceStatusResponse retVal;
	retVal.status = sdl::V1_0::agentino::ServiceStatus::STOPPING;

	imtclientgql::IAsyncGqlRequestTokenPtr token = SendModelRequestAsync<sdl::V1_0::agentino::CServiceStatusResponse>(
				gqlRequest,
				[this](sdl::V1_0::agentino::CServiceStatusResponse response, QString err) {
					Q_UNUSED(response);
					if (!err.isEmpty()){
						SendErrorMessage(0, err, "CServiceControllerProxyComp");
					}
				});
	if (!token.IsValid()){
		errorMessage = QStringLiteral("AsyncApiClient failed to dispatch StopService");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	errorMessage.clear();
	return retVal;
}


sdl::V1_0::imtbase::CRemovedNotificationPayload CServiceControllerProxyComp::OnServicesRemove(
			const sdl::V1_0::agentino::CServicesRemoveGqlRequest& removeServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	sdl::V1_0::agentino::ServicesRemoveRequestArguments arguments = removeServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	QByteArrayList serviceIds;
	if (arguments.input->elementIds){
		serviceIds = (*arguments.input->elementIds).ToList();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::imtbase::CRemovedNotificationPayload retVal = SendModelRequest<sdl::V1_0::imtbase::CRemovedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		// Do not purge the mirror when the agent is offline — the agent still owns the
		// service and would re-publish it on reconnect (same rule as OnRemoveElements).
		const QString offlineMessage = QStringLiteral(
					"Cannot delete the service while the agent is disconnected. "
					"Reconnect the agent and try again.");
		const QString rewritten = !serviceIds.isEmpty()
					? DescribeProxyError(serviceIds.first(), errorMessage)
					: errorMessage;
		if (rewritten.contains(QStringLiteral("disconnected"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("disconnect"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("offline"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("timeout"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("not connected"), Qt::CaseInsensitive)){
			errorMessage = offlineMessage;
		}
		else if (rewritten != errorMessage){
			errorMessage = rewritten;
		}
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	// Agent accepted delete — drop mirror entry optimistically.
	m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()));
	RemoveServiceStatuses(serviceIds);

	imtsdl::TElementList<QByteArray> removedIds;
	for (const QByteArray& serviceId: serviceIds){
		removedIds << serviceId;
	}
	retVal.elementIds = removedIds;

	return retVal;
}


sdl::V1_0::imtbase::CRemoveElementsPayload CServiceControllerProxyComp::OnRemoveElements(
			const sdl::V1_0::imtbase::CRemoveElementsGqlRequest& removeElementsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::imtbase::CRemoveElementsPayload response;
	response.success = false;

	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return response;
	}

	sdl::V1_0::imtbase::RemoveElementsRequestArguments arguments = removeElementsRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->elementIds.has_value()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		return response;
	}

	QByteArrayList serviceIds = (*arguments.input->elementIds).ToList();
	if (serviceIds.isEmpty()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		return response;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");
	if (agentId.isEmpty()){
		errorMessage = QStringLiteral(
					"Cannot delete the service: agent id (clientid) is missing from the request");
		response.success = false;
		return response;
	}

	// Forward to the owning Agent (source of truth). Never delete only from the server
	// mirror while the agent is offline — after reconnect the agent would re-sync the
	// service and it would reappear on Topology.
	sdl::V1_0::imtbase::CRemoveElementsPayload retVal =
				SendModelRequest<sdl::V1_0::imtbase::CRemoveElementsPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		// Prefer a clear offline message when the forward failed because the agent is down.
		const QString offlineMessage = QStringLiteral(
					"Cannot delete the service while the agent is disconnected. "
					"Reconnect the agent and try again.");
		const QString rewritten = DescribeProxyError(serviceIds.first(), errorMessage);
		if (rewritten.contains(QStringLiteral("disconnected"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("disconnect"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("offline"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("timeout"), Qt::CaseInsensitive)
					|| errorMessage.contains(QStringLiteral("not connected"), Qt::CaseInsensitive)){
			errorMessage = offlineMessage;
		}
		else if (rewritten != errorMessage){
			errorMessage = rewritten;
		}
		// Keep errorMessage for the GraphQL error payload (client shows Error dialog).
		retVal.success = false;
		return retVal;
	}

	// Agent accepted the delete — optimistically drop the mirror entry so GetTopology /
	// list UIs update even if NotifyAgentServicesCollectionChanged is delayed.
	m_serviceManagerCompPtr->RemoveServices(
				agentId,
				imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()));
	RemoveServiceStatuses(serviceIds);
	retVal.success = true;

	return retVal;
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnGetServiceStatus(
			const sdl::V1_0::agentino::CGetServiceStatusGqlRequest& getServiceStatusRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	sdl::V1_0::agentino::CServiceStatusResponse retVal = SendModelRequest<sdl::V1_0::agentino::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	return retVal;
}


sdl::V1_0::agentino::CUpdateConnectionUrlResponse CServiceControllerProxyComp::OnUpdateConnectionUrl(
			const sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest& /*updateConnectionUrlRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	return sdl::V1_0::agentino::CUpdateConnectionUrlResponse();
}


sdl::V1_0::agentino::CSetOutputConnectionResponse CServiceControllerProxyComp::OnSetOutputConnection(
			const sdl::V1_0::agentino::CSetOutputConnectionGqlRequest& setOutputConnectionRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSetOutputConnectionResponse response;
	response.succesful = false;

	sdl::V1_0::agentino::SetOutputConnectionRequestArguments arguments = setOutputConnectionRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->serviceId.has_value() || !arguments.input->connectionId.has_value()){
		errorMessage = QString("SetOutputConnection input is invalid");

		return response;
	}

	QString dependantConnectionId;
	if (arguments.input->dependantConnectionId.has_value()){
		dependantConnectionId = *arguments.input->dependantConnectionId;
	}

	sdl::V1_0::imtbase::CServerConnectionParam resolvedConnectionParam;
	if (!dependantConnectionId.isEmpty()){
		// Cross-agent resolution: only the server can see across the whole fleet.
		istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> resolvedPtr =
					GetDependantServerConnectionParam(dependantConnectionId.toUtf8());
		if (!resolvedPtr.IsValid()){
			errorMessage = QString("Producer connection '%1' was not found").arg(dependantConnectionId);

			return response;
		}

		if (!agentinodata::GetRepresentationFromServerConnectionParam(*resolvedPtr.GetPtr(), resolvedConnectionParam)){
			errorMessage = QString("Unable to resolve producer connection '%1'").arg(dependantConnectionId);

			return response;
		}
	}
	else{
		resolvedConnectionParam.host = QStringLiteral("localhost");
		resolvedConnectionParam.httpPort = -1;
		resolvedConnectionParam.wsPort = -1;
	}

	// Forward to the owning agent with the resolved param attached - an agent cannot
	// see other agents' services, so it must not be asked to resolve this itself.
	sdl::V1_0::agentino::SetOutputConnectionRequestArguments forwardArguments = arguments;
	forwardArguments.input->connectionParam = resolvedConnectionParam;

	imtgql::CGqlRequest forwardRequest;
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", gqlRequest.GetHeader("clientid"));
	gqlContextPtr->SetHeaders(headers);
	forwardRequest.SetGqlContext(gqlContextPtr);

	if (!sdl::V1_0::agentino::CSetOutputConnectionGqlRequest::SetupGqlRequest(forwardRequest, forwardArguments)){
		errorMessage = QString("Unable to build the forwarded request");

		return response;
	}

	response = SendModelRequest<sdl::V1_0::agentino::CSetOutputConnectionResponse>(forwardRequest, errorMessage);

	return response;
}


sdl::V1_0::agentino::CPluginInfo CServiceControllerProxyComp::OnLoadPlugin(
			const sdl::V1_0::agentino::CLoadPluginGqlRequest& loadPluginRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

		return sdl::V1_0::agentino::CPluginInfo();
	}

	sdl::V1_0::agentino::CPluginInfo retVal = SendModelRequest<sdl::V1_0::agentino::CPluginInfo>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		// LoadPluginInput only carries servicePath - the service id travels as the
		// "serviceid" header (ServiceEditorWrap's dataScope-based getHeaders()).
		errorMessage = DescribeProxyError(gqlRequest.GetHeader("serviceid"), errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CPluginInfo();
	}

	// The plugin's output slots are returned as-is; candidate producers for them are
	// fetched separately via AvailableConnections when the editor needs them.
	return retVal;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerProxyComp::OnGetServiceSettings(
			const sdl::V1_0::agentino::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::GetServiceSettingsRequestArguments arguments = getServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	sdl::V1_0::agentino::CServiceSettingsPayload retVal = SendModelRequest<sdl::V1_0::agentino::CServiceSettingsPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		const QByteArray serviceId = arguments.input->serviceId.has_value() ? *arguments.input->serviceId : QByteArray();
		errorMessage = DescribeProxyError(serviceId, errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	return retVal;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerProxyComp::OnUpdateServiceSettings(
			const sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::UpdateServiceSettingsRequestArguments arguments = updateServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	sdl::V1_0::agentino::CServiceSettingsPayload retVal = SendModelRequest<sdl::V1_0::agentino::CServiceSettingsPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		const QByteArray serviceId = arguments.input->serviceId.has_value() ? *arguments.input->serviceId : QByteArray();
		errorMessage = DescribeProxyError(serviceId, errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	return retVal;
}


// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)

QJsonObject CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	if (sdl::V1_0::agentino::CGetServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CGetServiceGqlRequest,
			sdl::V1_0::agentino::CServiceData>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CUpdateServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CUpdateServiceGqlRequest,
			sdl::V1_0::imtbase::CUpdatedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CAddServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CAddServiceGqlRequest,
			sdl::V1_0::imtbase::CAddedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnAddService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CStartServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CStartServiceGqlRequest,
			sdl::V1_0::agentino::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStartService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CStopServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CStopServiceGqlRequest,
			sdl::V1_0::agentino::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStopService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CServicesRemoveGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CServicesRemoveGqlRequest,
			sdl::V1_0::imtbase::CRemovedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnServicesRemove(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::imtbase::CRemoveElementsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::imtbase::CRemoveElementsGqlRequest,
			sdl::V1_0::imtbase::CRemoveElementsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnRemoveElements(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CLoadPluginGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CLoadPluginGqlRequest,
			sdl::V1_0::agentino::CPluginInfo>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnLoadPlugin(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CGetServiceSettingsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CGetServiceSettingsGqlRequest,
			sdl::V1_0::agentino::CServiceSettingsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetServiceSettings(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest,
			sdl::V1_0::agentino::CServiceSettingsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateServiceSettings(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CAvailableConnectionsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CAvailableConnectionsGqlRequest,
			sdl::V1_0::agentino::CAvailableConnectionsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnAvailableConnections(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CSetOutputConnectionGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CSetOutputConnectionGqlRequest,
			sdl::V1_0::agentino::CSetOutputConnectionResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnSetOutputConnection(req, gqlReq, err);
			});
	}
	if (commandId == QByteArrayLiteral("NotifyAgentServicesCollectionChanged")){
		return HandleAgentServiceCollectionNotify(gqlRequest, errorMessage);
	}

	return QJsonObject();
}


bool CServiceControllerProxyComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	if (BaseClass::IsRequestSupported(gqlRequest)){
		return true;
	}

	if (gqlRequest.GetCommandId() == QByteArrayLiteral("NotifyAgentServicesCollectionChanged")){
		return true;
	}

	// The generic 'RemoveElements' command (imtbase collection schema) isn't part of
	// Services.sdl, so CServicesGqlHandlerCompBase never recognizes it - accept it here
	// too, but only for the 'Services' collection, so this proxy doesn't steal the
	// command from unrelated collections.
	if (gqlRequest.GetCommandId() != sdl::V1_0::imtbase::CRemoveElementsGqlRequest::GetCommandId()){
		return false;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	if (inputParamPtr == nullptr){
		return false;
	}

	return inputParamPtr->GetParamArgumentValue("collectionId").toByteArray() == QByteArrayLiteral("Services");
}


QJsonObject CServiceControllerProxyComp::HandleAgentServiceCollectionNotify(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QJsonObject response;
	response.insert(QStringLiteral("success"), false);

	// Agent identity: WebSocket clientid header (agent is the caller).
	QByteArray agentId = gqlRequest.GetHeader(QByteArrayLiteral("clientid"));
	if (agentId.isEmpty()){
		const imtgql::IGqlContext* contextPtr = gqlRequest.GetRequestContext();
		if (contextPtr != nullptr){
			agentId = contextPtr->GetHeaders().value(QByteArrayLiteral("clientid"));
		}
	}

	if (agentId.isEmpty()){
		errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing clientid (agent id)");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return response;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject(QByteArrayLiteral("input"));
	if (inputParamPtr == nullptr){
		errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing input");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return response;
	}

	const QString typeOperation = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("typeOperation")).toString();
	const QByteArray itemId = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("itemId")).toByteArray();
	const QString itemIdsJoined = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("itemIds")).toString();

	QString syncError;
	bool ok = false;

	if (typeOperation == QStringLiteral("removed")){
		QByteArrayList serviceIds;
		const QStringList parts = itemIdsJoined.split(QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const QString& part: parts){
			serviceIds << part.toUtf8();
		}
		if (serviceIds.isEmpty() && !itemId.isEmpty()){
			serviceIds << itemId;
		}

		if (serviceIds.isEmpty()){
			// Empty ids (historical CChangeNotifier copy bug) — fall back to full agent reconcile
			// so topology does not keep ghost services with status UNDEFINED.
			SendErrorMessage(
						0,
						QStringLiteral("NotifyAgentServicesCollectionChanged: removed without item ids — reconciling agent"),
						"CServiceControllerProxyComp");
			ok = const_cast<CServiceControllerProxyComp*>(this)->SyncAgentServicesInMirror(agentId, syncError);
		}
		else{
			// const_cast: Sync methods are non-const on the synchronizer interface used by live path.
			ok = const_cast<CServiceControllerProxyComp*>(this)->RemoveServicesInMirror(agentId, serviceIds, syncError);
		}
	}
	else{
		// inserted / updated / default
		if (itemId.isEmpty()){
			errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing itemId");
			SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

			return response;
		}

		// Do NOT call SyncServiceInMirror here. The agent is blocked in a sync
		// SendRequest waiting for this notify response; SyncServiceInMirror does a
		// blocking GetService back to the same agent → deadlock / client timeout
		// ("Live service sync notify failed (inserted / <id>)").
		// ACK first, then GetService+mirror on a deferred pass.
		QueueServiceSync(agentId, itemId);
		ok = true;
	}

	if (!ok){
		errorMessage = syncError;
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		// Empty object → GQL transport layer treats as handler error with errorMessage.
		return QJsonObject();
	}

	errorMessage.clear();
	response.insert(QStringLiteral("success"), true);

	return response;
}


// private methods

template<class SdlGqlRequest, class SdlResponse>
QJsonObject CServiceControllerProxyComp::CreateResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage,
			std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	SdlGqlRequest serviceGqlRequest(gqlRequest, true);

	Q_ASSERT(serviceGqlRequest.IsValid());
	if (!serviceGqlRequest.IsValid()){
		return QJsonObject();
	}

	SdlResponse retVal = func(serviceGqlRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return QJsonObject();
	}

	QJsonObject resultObj;
	if (!retVal.WriteToJsonObject(resultObj)){
		errorMessage = QString("Unable to create response for command '%1'. Error: Writing to JSON object failed").arg(qPrintable(commandId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return QJsonObject();
	}

	return resultObj;
}


bool CServiceControllerProxyComp::SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceStatusCollection' was not set", "CServiceStatusControllerProxyComp");
		return false;
	}

	QByteArrayList serviceIds = m_serviceStatusCollectionCompPtr->GetElementIds();
	if (serviceIds.contains(serviceId)){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
			agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
			if (serviceStatusInfoPtr == nullptr){
				return false;
			}

			// Skip identical status — each SetObjectData fans out OnServiceStatusChanged.
			if (serviceStatusInfoPtr->GetServiceStatus() == status){
				return true;
			}

			serviceStatusInfoPtr->SetServiceStatus(status);

			if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr)){
				return false;
			}
		}
	}
	else{
		istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
		serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

		serviceStatusInfoPtr->SetServiceStatus(status);
		serviceStatusInfoPtr->SetServiceId(serviceId);

		QByteArray retVal = m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
		if (retVal.isEmpty()){
			return false;
		}
	}

	return true;
}


bool CServiceControllerProxyComp::SetServiceStatus(const QByteArray& serviceId, sdl::V1_0::agentino::ServiceStatus status) const
{
	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	if (status == sdl::V1_0::agentino::ServiceStatus::STARTING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STARTING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::NOT_RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_RUNNING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::STOPPING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STOPPING;
	}

	return SetServiceStatus(serviceId, serviceStatus);
}


QString CServiceControllerProxyComp::DescribeProxyError(const QByteArray& serviceId, const QString& errorMessage) const
{
	if (errorMessage.isEmpty() || serviceId.isEmpty() || !m_serviceStatusCollectionCompPtr.IsValid()){
		return errorMessage;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		return errorMessage;
	}

	agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
	if (serviceStatusInfoPtr == nullptr
				|| serviceStatusInfoPtr->GetServiceStatus() != agentinodata::IServiceStatusInfo::SS_UNDEFINED){
		return errorMessage;
	}

	return QStringLiteral("Agent is disconnected");
}


QList<sdl::V1_0::agentino::CDependantConnectionInfo> CServiceControllerProxyComp::BuildAvailableConnections(
			const QByteArray& connectionUsageId) const
{
	// Offer every service in the whole fleet that publishes this connection — the one thing
	// only the server can do, since an agent cannot see its peers. Each entry is addressed
	// by a ServiceEndpointId, so two producers of the same connection type stay distinct.
	QList<sdl::V1_0::agentino::CDependantConnectionInfo> available;
	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		return available;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		agentinodata::AppendAvailableConnectionsFromServiceCollection(
					*serviceCollectionPtr,
					connectionUsageId,
					available);
	}

	return available;
}


sdl::V1_0::agentino::CAvailableConnectionsPayload CServiceControllerProxyComp::OnAvailableConnections(
			const sdl::V1_0::agentino::CAvailableConnectionsGqlRequest& availableConnectionsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CAvailableConnectionsPayload response;
	response.outputConnections.Emplace();

	const sdl::V1_0::agentino::AvailableConnectionsRequestArguments arguments =
				availableConnectionsRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->connectionUsageIds.has_value()){
		return response;
	}

	// One group per requested output slot; the candidate scan runs over the mirror only,
	// so this is answered entirely on the server without forwarding to any agent.
	QList<sdl::V1_0::agentino::COutputConnectionCandidates> groups;
	const QByteArrayList usageIds = (*arguments.input->connectionUsageIds).ToList();
	for (const QByteArray& usageId : usageIds){
		sdl::V1_0::agentino::COutputConnectionCandidates group;
		group.connectionUsageId = usageId;
		group.candidates.Emplace();
		group.candidates->FromList(BuildAvailableConnections(usageId));
		groups << group;
	}

	response.outputConnections->FromList(groups);

	return response;
}


istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> CServiceControllerProxyComp::GetDependantServerConnectionParam(const QByteArray& dependantId) const
{
	// The endpoint id says which service publishes it and which of its connections it
	// is, so this is a direct lookup instead of a walk over every agent's services.
	QByteArray serviceId;
	QByteArray connectionId;
	if (!agentinodata::ServiceEndpointId::Parse(dependantId, serviceId, connectionId)){
		return nullptr;
	}

	agentinodata::IServiceInfo* serviceInfoPtr = FindMirroredService(serviceId);
	if (serviceInfoPtr == nullptr){
		return nullptr;
	}

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
	if (connectionCollectionPtr == nullptr){
		return nullptr;
	}

	// The connection part is either an input-connection element of the producer...
	imtbase::IObjectCollection::DataPtr connectionDataPtr;
	if (connectionCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
		imtservice::CUrlConnectionParam* connectionParamPtr =
					dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
		if (connectionParamPtr == nullptr){
			return nullptr;
		}

		istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr;
		serverConnectionInterfacePtr.MoveCastedPtr(connectionParamPtr->CloneMe());

		return serverConnectionInterfacePtr;
	}

	// ...or the uuid of one of its incoming connections (an alternative published address).
	const imtbase::ICollectionInfo::Ids inputIds = connectionCollectionPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& inputId : inputIds){
		imtbase::IObjectCollection::DataPtr inputDataPtr;
		if (!connectionCollectionPtr->GetObjectData(inputId, inputDataPtr)){
			continue;
		}

		const imtservice::CUrlConnectionParam* inputParamPtr =
					dynamic_cast<const imtservice::CUrlConnectionParam*>(inputDataPtr.GetPtr());
		if (inputParamPtr == nullptr){
			continue;
		}

		const imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections =
					inputParamPtr->GetIncomingConnections();
		for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incoming : incomingConnections){
			if (incoming.GetObjectUuid() == connectionId){
				istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr;
				serverConnectionInterfacePtr.MoveCastedPtr(incoming.CloneMe());

				return serverConnectionInterfacePtr;
			}
		}
	}

	return nullptr;
}


agentinodata::IServiceInfo* CServiceControllerProxyComp::FindMirroredService(const QByteArray& serviceId) const
{
	if (serviceId.isEmpty() || !m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		return nullptr;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		if (m_serviceManagerCompPtr->ServiceExists(agentId, serviceId)){
			return m_serviceManagerCompPtr->GetService(agentId, serviceId);
		}
	}

	return nullptr;
}


QList<CServiceControllerProxyComp::ChangedConnectionUrl> CServiceControllerProxyComp::GetChangedConnectionUrl(
			const sdl::V1_0::agentino::CServiceData& serviceData1,
			const sdl::V1_0::agentino::CServiceData& serviceData2) const
{
	QList<ChangedConnectionUrl> retVal;

	if (!serviceData1.inputConnections || !serviceData2.inputConnections){
		return retVal;
	}

	auto toChangedConnectionUrl = [](const QByteArray& addressableId, const sdl::V1_0::imtbase::CServerConnectionParam& param){
		ChangedConnectionUrl changed;
		changed.addressableId = addressableId;
		changed.host = param.host.has_value() ? *param.host : QString();
		changed.isSecure = param.isSecure.has_value() && *param.isSecure;
		changed.httpPort = param.httpPort.has_value() ? *param.httpPort : -1;
		changed.httpPath = param.httpPath.has_value() ? *param.httpPath : QString();
		changed.wsPort = param.wsPort.has_value() ? *param.wsPort : -1;

		return changed;
	};

	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections1 = *serviceData1.inputConnections;
	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections2 = *serviceData2.inputConnections;

	for (int i = 0; i < connections1->size() && i < connections2->size(); i++){
		istd::TNullableValue<sdl::V1_0::agentino::CInputConnection> connection1 = (*connections1)[i];
		istd::TNullableValue<sdl::V1_0::agentino::CInputConnection> connection2 = (*connections2)[i];

		sdl::V1_0::imtbase::CServerConnectionParam connectionParam1 = *connection1->connectionParam;
		sdl::V1_0::imtbase::CServerConnectionParam connectionParam2 = *connection2->connectionParam;

		if (*connectionParam1.host != *connectionParam2.host ||
			*connectionParam1.httpPort != *connectionParam2.httpPort ||
			*connectionParam1.wsPort != *connectionParam2.wsPort){
			retVal << toChangedConnectionUrl(*connection1->id, connectionParam2);
		}

		// Extern addresses are separately addressable (ServiceEndpointId of the extern's
		// own uuid, see AppendAvailableConnectionsFromServiceCollection) - a consumer can be
		// linked to one specific extern address independently of the main one, so it must be
		// told when THAT ONE changes, not just when the main address does.
		if (!connection1->externConnectionList || !connection2->externConnectionList){
			continue;
		}

		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CExternConnectionInfo>> externs1 = *connection1->externConnectionList;
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CExternConnectionInfo>> externs2 = *connection2->externConnectionList;

		for (const istd::TNullableValue<sdl::V1_0::agentino::CExternConnectionInfo>& externNew : *externs2){
			if (!externNew->id || !externNew->connectionParam){
				continue;
			}

			for (const istd::TNullableValue<sdl::V1_0::agentino::CExternConnectionInfo>& externOld : *externs1){
				if (!externOld->id || *externOld->id != *externNew->id || !externOld->connectionParam){
					continue;
				}

				const sdl::V1_0::imtbase::CServerConnectionParam& oldParam = *externOld->connectionParam;
				const sdl::V1_0::imtbase::CServerConnectionParam& newParam = *externNew->connectionParam;
				if (*oldParam.host != *newParam.host ||
					*oldParam.httpPort != *newParam.httpPort ||
					*oldParam.wsPort != *newParam.wsPort){
					retVal << toChangedConnectionUrl(*externNew->id, newParam);
				}

				break;
			}
		}
	}

	return retVal;
}


QVector<CServiceControllerProxyComp::ConsumerRef> CServiceControllerProxyComp::FindConsumersOfEndpoint(
			const QByteArray& endpointId) const
{
	QVector<ConsumerRef> retVal;
	if (endpointId.isEmpty() || !m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		return retVal;
	}

	// Consumers are found by scanning: the link lives inside each consumer descriptor,
	// so there is no index to ask. This runs only when a producer URL actually changed.
	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		imtbase::IObjectCollection* serviceCollectionPtr =
					m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			agentinodata::IServiceInfo* serviceInfoPtr =
						dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			imtbase::IObjectCollection* linksPtr = serviceInfoPtr->GetDependantServiceConnections();
			if (linksPtr == nullptr){
				continue;
			}

			const imtbase::ICollectionInfo::Ids linkIds = linksPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& linkId : linkIds){
				imtbase::IObjectCollection::DataPtr linkDataPtr;
				if (!linksPtr->GetObjectData(linkId, linkDataPtr)){
					continue;
				}

				const imtservice::CUrlConnectionLinkParam* linkPtr =
							dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(linkDataPtr.GetPtr());
				if (linkPtr != nullptr && linkPtr->GetDependantServiceConnectionId() == endpointId){
					ConsumerRef consumer;
					consumer.agentId = agentId;
					consumer.serviceId = serviceId;
					consumer.slot = linkId;
					retVal << consumer;
					break;
				}
			}
		}
	}

	return retVal;
}


bool CServiceControllerProxyComp::UpdateConnectionForService(
			const QByteArray& serviceId,
			const QByteArray& agentId,
			const QByteArray& connectionId,
			const sdl::V1_0::imtbase::CServerConnectionParam& connectionParam) const
{
	sdl::V1_0::agentino::UpdateConnectionUrlRequestArguments arguments;
	arguments.input = sdl::V1_0::agentino::CConnectionUrlInput();
	arguments.input->serviceId = serviceId;
	arguments.input->connectionId = connectionId;
	arguments.input->connectionParam = connectionParam;

	imtgql::CGqlRequest gqlRequest;
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		return false;
	}

	QString errorMessage;
	sdl::V1_0::agentino::CUpdateConnectionUrlResponse retVal = SendModelRequest<sdl::V1_0::agentino::CUpdateConnectionUrlResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return false;
	}

	if (retVal.succesful.has_value()){
		return *retVal.succesful;
	}

	return false;
}


QByteArrayList CServiceControllerProxyComp::GetMirrorServiceIds(const QByteArray& agentId) const
{
	QByteArrayList retVal;

	if (!m_agentCollectionCompPtr.IsValid() || agentId.isEmpty()){
		return retVal;
	}

	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (!m_agentCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
		return retVal;
	}

	agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
	if (agentInfoPtr == nullptr){
		return retVal;
	}

	imtbase::IObjectCollection* serviceCollectionPtr =
				m_serviceManagerCompPtr.IsValid()
							? m_serviceManagerCompPtr->GetServiceCollection(agentId)
							: nullptr;
	if (serviceCollectionPtr == nullptr){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& id : ids){
		retVal << id;
	}

	return retVal;
}


void CServiceControllerProxyComp::RemoveServiceStatuses(const QByteArrayList& serviceIds) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid() || serviceIds.isEmpty()){
		return;
	}

	const QByteArrayList existingStatusIds = m_serviceStatusCollectionCompPtr->GetElementIds();
	imtbase::ICollectionInfo::Ids toRemove;
	for (const QByteArray& serviceId : serviceIds){
		if (existingStatusIds.contains(serviceId)){
			toRemove << serviceId;
		}
	}

	if (!toRemove.isEmpty()){
		m_serviceStatusCollectionCompPtr->RemoveElements(toRemove);
	}
}


// RemoveStaleMirrorServicesByPath deleted (R1.2): dual-write drift papering no longer needed.
// Full agent ServicesList reconcile removes ids absent on the agent.


void CServiceControllerProxyComp::AppendServicesListFields(imtgql::CGqlRequest& gqlRequest)
{
	// Matches agent ListObjects/GetInformationIds("items") expectations.
	imtgql::CGqlFieldObject itemsFields;
	itemsFields.InsertField("id");
	itemsFields.InsertField("typeId");
	itemsFields.InsertField("name");
	itemsFields.InsertField("description");
	itemsFields.InsertField("path");
	itemsFields.InsertField("status");
	itemsFields.InsertField("version");
	gqlRequest.AddField("items", itemsFields);
}


void CServiceControllerProxyComp::AppendServiceDataFields(imtgql::CGqlRequest& gqlRequest)
{
	// Minimal set required by GetServiceFromRepresentation + mirror storage.
	gqlRequest.AddSimpleField("id");
	gqlRequest.AddSimpleField("name");
	gqlRequest.AddSimpleField("description");
	gqlRequest.AddSimpleField("path");
	gqlRequest.AddSimpleField("arguments");
	gqlRequest.AddSimpleField("serviceTypeId");
	gqlRequest.AddSimpleField("enableVerbose");
	gqlRequest.AddSimpleField("isAutoStart");
	gqlRequest.AddSimpleField("tracingLevel");
	gqlRequest.AddSimpleField("startScript");
	gqlRequest.AddSimpleField("stopScript");
	gqlRequest.AddSimpleField("settingsPath");
	gqlRequest.AddSimpleField("version");
	gqlRequest.AddSimpleField("status");

	imtgql::CGqlFieldObject connectionParamFields;
	connectionParamFields.InsertField("host");
	connectionParamFields.InsertField("httpPort");
	connectionParamFields.InsertField("wsPort");

	imtgql::CGqlFieldObject inputConnectionFields;
	inputConnectionFields.InsertField("id");
	inputConnectionFields.InsertField("connectionName");
	inputConnectionFields.InsertField("description");
	inputConnectionFields.InsertField("serviceTypeId");
	inputConnectionFields.InsertField("connectionParam", connectionParamFields);

	imtgql::CGqlFieldObject outputConnectionFields;
	outputConnectionFields.InsertField("id");
	outputConnectionFields.InsertField("connectionName");
	outputConnectionFields.InsertField("description");
	outputConnectionFields.InsertField("serviceName");
	outputConnectionFields.InsertField("serviceTypeId");
	outputConnectionFields.InsertField("dependantConnectionId");
	outputConnectionFields.InsertField("connectionParam", connectionParamFields);

	gqlRequest.AddField("inputConnections", inputConnectionFields);
	gqlRequest.AddField("outputConnections", outputConnectionFields);
}


} // namespace agentinogql

