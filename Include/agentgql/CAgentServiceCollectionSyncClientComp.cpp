// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentServiceCollectionSyncClientComp.h>


// Qt includes
#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

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
		else{
			qDebug() << "CAgentServiceCollectionSyncClientComp: observer attached to ServiceCollection"
					 << "thread=" << QThread::currentThread();
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
	qDebug() << "CAgentServiceCollectionSyncClientComp::OnUpdate"
			 << "thread=" << QThread::currentThread()
			 << "main=" << (QThread::currentThread() == thread())
			 << "CF_ADDED=" << changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED)
			 << "CF_REMOVED=" << changeSet.Contains(imtbase::ICollectionInfo::CF_REMOVED)
			 << "CF_ELEMENT_RENAMED=" << changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_RENAMED)
			 << "CF_ELEMENT_DESCRIPTION_CHANGED=" << changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_DESCRIPTION_CHANGED)
			 << "CF_ELEMENT_STATE=" << changeSet.Contains(imtbase::ICollectionInfo::CF_ELEMENT_STATE)
			 << "CF_OBJECT_DATA_CHANGED=" << changeSet.Contains(imtbase::IObjectCollection::CF_OBJECT_DATA_CHANGED);

	if (!m_gqlClientCompPtr.IsValid()){
		SendErrorMessage(0, "GqlClient is not set — cannot notify server", "CAgentServiceCollectionSyncClientComp");

		return;
	}

	// Insert: CN_ELEMENT_INSERTED is set before CChangeNotifier in CObjectCollectionBase::InsertNewObject.
	if (changeSet.Contains(imtbase::ICollectionInfo::CF_ADDED)){
		const QByteArray itemId = ChangeInfoToItemId(
					changeSet.GetChangeInfo(imtbase::ICollectionInfo::CN_ELEMENT_INSERTED));
		qDebug() << "CAgentServiceCollectionSyncClientComp::OnUpdate CF_ADDED itemId=" << itemId;
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
		qDebug() << "CAgentServiceCollectionSyncClientComp::OnUpdate CF_REMOVED itemIds=" << itemIds;
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

	qDebug() << "CAgentServiceCollectionSyncClientComp::OnUpdate updated"
			 << updateKind << "itemId=" << itemId;
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
	qDebug() << "CAgentServiceCollectionSyncClientComp: queue NotifyServer"
			 << typeOperation << itemId << itemIds
			 << "fromThread=" << QThread::currentThread()
			 << "toThread=" << thread();

	const bool queued = QMetaObject::invokeMethod(
				this,
				[this, typeOperation, itemId, itemIds](){
					qDebug() << "CAgentServiceCollectionSyncClientComp::NotifyServer (main thread)"
							 << typeOperation << itemId << itemIds
							 << "thread=" << QThread::currentThread();
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

	imtgql::CGqlFieldObject resultFields;
	resultFields.InsertField(QByteArrayLiteral("success"));
	gqlRequestPtr->AddField(QByteArrayLiteral("data"), resultFields);

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

	qDebug() << "CAgentServiceCollectionSyncClientComp::NotifyServer"
			 << typeOperation << itemId << itemIds;

	imtclientgql::IGqlClient::GqlRequestPtr requestPtr(gqlRequestPtr);
	imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_gqlClientCompPtr->SendRequest(requestPtr);
	if (!responsePtr.IsValid()){
		SendErrorMessage(
					0,
					QString("Live service sync notify failed (%1 / %2)").arg(typeOperation, QString::fromUtf8(itemId)),
					"CAgentServiceCollectionSyncClientComp");
	}
	else{
		qDebug() << "CAgentServiceCollectionSyncClientComp::NotifyServer OK"
				 << typeOperation << itemId << itemIds;
	}
}


} // namespace agentgql
