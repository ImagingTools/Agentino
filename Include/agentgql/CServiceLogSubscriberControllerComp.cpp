// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceLogSubscriberControllerComp.h>


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
	m_timer.setInterval(1000);
	m_timer.start();
}

void CServiceLogSubscriberControllerComp::OnComponentDestroyed()
{
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
	QList<QByteArray> keys = m_pluginMap.keys();
	for (const QByteArray& serviceId: keys){
		if (!serviceIds.contains(serviceId)){
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

			istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

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
						messageCollectionFactoryPtr = pluginPtr->GetObjectCollectionFactory();

						break;
					}
				}
				if (messageCollectionFactoryPtr != nullptr){
					istd::TUniqueInterfacePtr<imtbase::IObjectCollection> messageCollection = messageCollectionFactoryPtr->CreateInstance();
					if (messageCollection.IsValid()){
						MessageStatusInfo& messageStatusInfo = m_servicesMessageInfo[serviceId];
						messageStatusInfo.messageCollectionPtr.FromUnique(messageCollection);
						messageStatusInfo.messageCount = 0;
					}
				}
			}
		}
	}

}


void CServiceLogSubscriberControllerComp::TimerUpdate()
{
	for (const QByteArray& serviceId: m_servicesMessageInfo.keys()){
		imtbase::IObjectCollection* messageCollection = m_servicesMessageInfo[serviceId].messageCollectionPtr.GetPtr();
		if (messageCollection != nullptr){
			int messageCount = messageCollection->GetElementsCount();
			if (m_servicesMessageInfo[serviceId].messageCount != messageCount){
				QString data = QString("{\"serviceid\": \"%1\"}").arg(qPrintable(serviceId));

				PublishData("OnServiceLogChanged", data.toUtf8());

				m_servicesMessageInfo[serviceId].messageCount = messageCount;
			}
		}
	}
}


} // namespace agentgql


