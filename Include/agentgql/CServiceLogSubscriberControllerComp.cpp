// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceLogSubscriberControllerComp.h>


// STL includes
#include <algorithm>

// Qt includes
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutexLocker>
#include <QtConcurrent/QtConcurrentRun>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


// protected methods

// reimplemented (icomp::CComponentBase)

void CServiceLogSubscriberControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_serviceCollectionModelCompPtr.IsValid()){
		m_serviceCollectionModelCompPtr->AttachObserver(this);
	}

	connect(&m_timer, &QTimer::timeout, this, &CServiceLogSubscriberControllerComp::TimerUpdate);
	m_timer.setSingleShot(true);
	m_timer.start(s_minPollIntervalMs);
}

void CServiceLogSubscriberControllerComp::OnComponentDestroyed()
{
	m_timer.stop();

	// The worker runs plugin code; it must finish before the plugins are unloaded.
	m_pollFuture.waitForFinished();

	if (m_serviceCollectionModelCompPtr.IsValid()){
		m_serviceCollectionModelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


// reimplemented (imod::CSingleModelObserverBase)

void CServiceLogSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& /*changeSet*/)
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return;
	}

	imtbase::ICollectionInfo::Ids serviceIds = m_serviceCollectionCompPtr->GetElementIds();

	QMutexLocker pluginMapLocker(&m_pluginMapMutex);

	QList<QByteArray> keys = m_pluginMap.keys();
	for (const QByteArray& serviceId: keys){
		if (!serviceIds.contains(serviceId)){
			m_pollFuture.waitForFinished();

			m_pluginMap.remove(serviceId);
			m_servicesMessageInfo.remove(serviceId);
		}
	}

	for (const QByteArray& serviceId: serviceIds){
		if (!m_pluginMap.contains(serviceId)){
			agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
			imtbase::IObjectCollection::DataPtr dataPtr;
			if (m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
				serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(dataPtr.GetPtr());
			}

			if (serviceInfoPtr == nullptr){
				continue;
			}

			QByteArray servicePath = serviceInfoPtr->GetServicePath();
			QByteArray serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceId];
			pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

			if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")){
				SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(qPrintable(serviceName)), "CServiceLogSubscriberControllerComp");
				m_pluginMap.remove(serviceId);
				m_servicesMessageInfo.remove(serviceId);

				continue;
			}

			if (m_pluginMap.contains(serviceId)){
				const imtservice::IObjectCollectionPlugin::IObjectCollectionFactory* messageCollectionFactoryPtr = nullptr;
				for (int index = 0; index < m_pluginMap[serviceId]->m_plugins.count(); index++){
					imtservice::IObjectCollectionPlugin* pluginPtr = m_pluginMap[serviceId]->m_plugins[index].pluginPtr;
					if (pluginPtr != nullptr){
						if (pluginPtr->GetPluginName() != fileInfo.baseName() + "Log"){
							continue;
						}

						messageCollectionFactoryPtr = pluginPtr->GetObjectCollectionFactory();

						break;
					}
				}
				if (messageCollectionFactoryPtr != nullptr){
					istd::TUniqueInterfacePtr<imtbase::IObjectCollection> messageCollection = messageCollectionFactoryPtr->CreateInstance();
					if (messageCollection.IsValid()){
						MessageStatusInfo& messageStatusInfo = m_servicesMessageInfo[serviceId];
						messageStatusInfo.messageCollectionPtr.FromUnique(std::move(messageCollection));
					}
				}
			}
		}
	}

}


void CServiceLogSubscriberControllerComp::TimerUpdate()
{
	bool hasSubscribers = false;
	{
		QMutexLocker locker(&m_mutex);
		hasSubscribers = !m_registeredSubscribers.isEmpty();
	}

	if (!hasSubscribers){
		m_timer.start(s_minPollIntervalMs);

		return;
	}

	QElapsedTimer pollTimer;
	pollTimer.start();

	QList<QPair<QByteArray, istd::TSharedInterfacePtr<imtbase::IObjectCollection>>> messageCollections;
	for (auto it = m_servicesMessageInfo.cbegin(); it != m_servicesMessageInfo.cend(); ++it){
		if (it->messageCollectionPtr.IsValid()){
			messageCollections.append(qMakePair(it.key(), it->messageCollectionPtr));
		}
	}

	// Counting a large service log takes tens of seconds; the main thread carries the server WebSocket.
	m_pollFuture = QtConcurrent::run([messageCollections](){
		MessageCounts messageCounts;
		for (const auto& messageCollection : messageCollections){
			messageCounts.insert(messageCollection.first, messageCollection.second->GetElementsCount());
		}

		return messageCounts;
	});

	m_pollFuture.then(this, [this, pollTimer](const MessageCounts& messageCounts){
		OnPollFinished(messageCounts, pollTimer.elapsed());
	});
}


// private methods

void CServiceLogSubscriberControllerComp::OnPollFinished(const MessageCounts& messageCounts, qint64 elapsedMs)
{
	for (auto it = messageCounts.cbegin(); it != messageCounts.cend(); ++it){
		auto infoIt = m_servicesMessageInfo.find(it.key());
		if (infoIt == m_servicesMessageInfo.end()){
			continue;
		}

		const int previousCount = infoIt->messageCount;
		infoIt->messageCount = it.value();

		if (previousCount >= 0 && previousCount != it.value()){
			QString data = QString("{\"serviceid\": \"%1\"}").arg(qPrintable(it.key()));

			PublishData("OnServiceLogCollectionChanged", data.toUtf8());
		}
	}

	// Each pass scans the whole log; leave the database mostly to the log page requests.
	const qint64 nextIntervalMs = std::clamp<qint64>(elapsedMs * 10, s_minPollIntervalMs, s_maxPollIntervalMs);
	m_timer.start(int(nextIntervalMs));
}


} // namespace agentgql


