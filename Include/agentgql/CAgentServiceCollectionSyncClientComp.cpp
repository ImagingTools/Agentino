// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentServiceCollectionSyncClientComp.h>


// Qt includes
#include <QtCore/QMetaObject>

// ImtCore includes
#include <imtbase/ICollectionInfo.h>
#include <imtbase/IObjectCollection.h>
#include <imtgql/CGqlRequest.h>
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlFieldObject.h>


namespace agentgql
{


const QByteArray CAgentServiceCollectionSyncClientComp::s_notifyCommandId = QByteArrayLiteral("NotifyAgentServicesCollectionChanged");


namespace
{


QByteArray ChangeInfoToItemId(const QVariant& changeInfo)
{
	if (!changeInfo.isValid()){
		return QByteArray();
	}

	if (changeInfo.userType() == QMetaType::QByteArray){
		return changeInfo.toByteArray();
	}

	if (changeInfo.userType() == QMetaType::QString){
		return changeInfo.toString().toUtf8();
	}

	return changeInfo.toByteArray();
}


QByteArrayList ExtractRemovedItemIds(const istd::IChangeable::ChangeSet& changeSet)
{
	QByteArrayList itemIds;

	const QVariant changeInfo = changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENTS_REMOVED);
	if (!changeInfo.isValid()){
		return itemIds;
	}

	const imtbase::ICollectionInfo::MultiElementNotifierInfo info =
				changeInfo.value<imtbase::ICollectionInfo::MultiElementNotifierInfo>();
	for (const imtbase::ICollectionInfo::Id& id: info.elementIds){
		if (!id.isEmpty()){
			itemIds << id;
		}
	}

	return itemIds;
}


} // namespace


// reimplemented (icomp::CComponentBase)

void CAgentServiceCollectionSyncClientComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	// Same attach path as CObjectCollectionChangeNotifierComp (working local notifier).
	if (m_objectCollectionModelCompPtr.IsValid()){
		if (!m_objectCollectionModelCompPtr->AttachObserver(this)){
			SendErrorMessage(
						0,
						"Unable to attach observer to ServiceCollection model",
						"CAgentServiceCollectionSyncClientComp");
		}
	}
	else{
		SendErrorMessage(
					0,
					"Attribute 'ServiceCollection' was not set or is not an IModel — live agent→server service sync is disabled",
					"CAgentServiceCollectionSyncClientComp");
	}
}


void CAgentServiceCollectionSyncClientComp::OnComponentDestroyed()
{
	if (m_objectCollectionModelCompPtr.IsValid()){
		m_objectCollectionModelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


// reimplemented (imod::CSingleModelObserverBase)

void CAgentServiceCollectionSyncClientComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!m_gqlClientCompPtr.IsValid()){
		SendErrorMessage(0, "GqlClient is not set — cannot notify server", "CAgentServiceCollectionSyncClientComp");

		return;
	}

	// Insert: CN_ELEMENT_INSERTED is set before CChangeNotifier in CObjectCollectionBase::InsertNewObject.
	if (changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED)){
		const QByteArray itemId = ChangeInfoToItemId(
					changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_INSERTED));
		if (itemId.isEmpty()){
			SendErrorMessage(
						0,
						"CF_ADDED without CN_ELEMENT_INSERTED — live insert sync skipped",
						"CAgentServiceCollectionSyncClientComp");
		}
		else{
			QueueNotifyServer(QStringLiteral("inserted"), itemId, QByteArrayList());
		}

		return;
	}

	// Remove: CN_ELEMENTS_REMOVED must contain MultiElementNotifierInfo with ids
	// (filled before CChangeNotifier copy in CObjectCollectionBase::RemoveElements).
	if (changeSet.Contains(imtbase::ICollectionInfo::CF_REMOVED)){
		const QByteArrayList itemIds = ExtractRemovedItemIds(changeSet);
		if (itemIds.isEmpty()){
			SendErrorMessage(
						0,
						"CF_REMOVED without element ids in CN_ELEMENTS_REMOVED — live remove sync skipped",
						"CAgentServiceCollectionSyncClientComp");
		}
		else{
			QueueNotifyServer(QStringLiteral("removed"), QByteArray(), itemIds);
		}

		return;
	}

	// Update paths: rename / description / enabled / SetObjectData.
	// Nested child-model edits re-emitted only as CF_DELEGATED are intentionally ignored
	// (no item id). Service GQL update uses SetObjectData → CF_OBJECT_DATA_CHANGED.
	QByteArray itemId;
	const char* updateKind = nullptr;

	if (changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_RENAMED)){
		itemId = ChangeInfoToItemId(changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_RENAMED));
		updateKind = "renamed";
	}
	else if (changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_DESCRIPTION_CHANGED)){
		itemId = ChangeInfoToItemId(
					changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_DESCRIPTION_CHANGED));
		updateKind = "description";
	}
	else if (changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_STATE)){
		itemId = ChangeInfoToItemId(changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_STATE));
		updateKind = "state";
	}
	else if (changeSet.Contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED)){
		itemId = ChangeInfoToItemId(
					changeSet.GetChangeInfo(imtbase::IObjectCollection::CN_OBJECT_DATA_CHANGED));
		updateKind = "objectData";
	}

	if (updateKind == nullptr){
		return;
	}

	if (itemId.isEmpty()){
		SendErrorMessage(
					0,
					QString("Collection update (%1) without item id — live update sync skipped").arg(updateKind),
					"CAgentServiceCollectionSyncClientComp");
	}
	else{
		QueueNotifyServer(QStringLiteral("updated"), itemId, QByteArrayList());
	}
}


void CAgentServiceCollectionSyncClientComp::QueueNotifyServer(
			const QString& typeOperation,
			const QByteArray& itemId,
			const QByteArrayList& itemIds)
{
	// AddService/RemoveElements run on a GQL worker thread; collection OnUpdate follows on that
	// same thread. QWebSocket / QSocketNotifier must only be touched on the client's thread
	// (main). Queue the actual SendRequest to this QObject's thread affinity.
	// Functor form avoids Q_ARG meta-type registration for QByteArrayList.
	const bool queued = QMetaObject::invokeMethod(
				this,
				[this, typeOperation, itemId, itemIds](){
					NotifyServer(typeOperation, itemId, itemIds);
				},
				Qt::QueuedConnection);

	if (!queued){
		SendErrorMessage(
					0,
					QString("Unable to queue live service sync notify (%1 / %2)")
								.arg(typeOperation, QString::fromUtf8(itemId)),
					"CAgentServiceCollectionSyncClientComp");
	}
}


void CAgentServiceCollectionSyncClientComp::NotifyServer(
			const QString& typeOperation,
			const QByteArray& itemId,
			const QByteArrayList& itemIds) const
{
	if (!m_gqlClientCompPtr.IsValid()){
		return;
	}

	imtgql::CGqlRequest* gqlRequestPtr = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_MUTATION, s_notifyCommandId);

	imtgql::CGqlParamObject input;
	input.InsertParam(QByteArrayLiteral("typeOperation"), typeOperation);
	if (!itemId.isEmpty()){
		input.InsertParam(QByteArrayLiteral("itemId"), QString::fromUtf8(itemId));
	}
	if (!itemIds.isEmpty()){
		input.InsertParam(QByteArrayLiteral("itemIds"), QString::fromUtf8(itemIds.join(';')));
	}
	gqlRequestPtr->AddParam(QByteArrayLiteral("input"), input);

	// Handler returns { success: true } at the command root (not nested under "data").
	gqlRequestPtr->AddSimpleField(QByteArrayLiteral("success"));

	// Ensure server can resolve agent id from headers (MT_QUERY path copies headers into HTTP request).
	if (m_clientIdCompPtr.IsValid()){
		const QByteArray clientId = m_clientIdCompPtr->GetText().toUtf8();
		if (!clientId.isEmpty()){
			imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
			imtgql::IGqlContext::Headers headers;
			headers.insert(QByteArrayLiteral("clientid"), clientId);
			gqlContextPtr->SetHeaders(headers);
			gqlRequestPtr->SetGqlContext(gqlContextPtr);
		}
	}

	// itemId for "removed" is always empty (the plural ids travel via itemIds instead) - use
	// whichever one is actually populated so the log names the real service(s).
	const QString displayId = itemIds.isEmpty() ? QString::fromUtf8(itemId) : QString::fromUtf8(itemIds.join(';'));

	// Fire-and-forget: the server mirror is refreshed via the deferred GetService/reconcile
	// path regardless of whether this notify's ACK ever comes back (and the periodic reconcile
	// catches anything this misses), so there is nothing useful to do with the response here -
	// waiting for it just held this component's thread hostage for up to SendRequest()'s 30s
	// budget on every service change, which is what showed up as spurious "no response from
	// server" timeouts under normal load. SendRequestNoWait() only reports whether the message
	// was handed to the socket, not whether the server acted on it.
	imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlRequestPtr);
	if (!m_gqlClientCompPtr->SendRequestNoWait(requestPtr)){
		SendErrorMessage(
					0,
					QString("Live service sync notify (%1 / %2) could not be sent to the server")
								.arg(typeOperation, displayId),
					"CAgentServiceCollectionSyncClientComp");
	}
}


} // namespace agentgql
