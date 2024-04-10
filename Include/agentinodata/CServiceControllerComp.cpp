#include <agentinodata/CServiceControllerComp.h>


// ACF includes
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceController)

QProcess::ProcessState  CServiceControllerComp::GetServiceStatus(const QByteArray& serviceId) const
{
	if (m_processMap.contains(serviceId)){
		return m_processMap[serviceId]->state();
	}

	return QProcess::NotRunning;
}


bool CServiceControllerComp::StartService(const QByteArray& serviceId)
{
	bool retVal = false;

	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(0);

		return retVal;
	}

	updateServiceVersion(serviceId);

	agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		Q_ASSERT(0);

		return retVal;
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	QByteArrayList serviceArguments = serviceInfoPtr->GetServiceArguments();

	QProcess* process = nullptr;

	if (m_processMap.contains(serviceId)){
		process = m_processMap.value(serviceId);

		bool isStopped = process->property("isStopped").toBool();
		if (isStopped){
			m_restartProcessing.append(serviceId);

			return true;
		}
	}
	else{
		process = new QProcess(this);
		m_processMap.insert(serviceId, process);
		connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

		process->setProgram(servicePath);
		QStringList arguments;
		for (const QByteArray& argument: serviceArguments){
			arguments << QString(argument);
		}
		process->setArguments(arguments);
	}

	process->start();

	return true;
}


bool CServiceControllerComp::StopService(const QByteArray& serviceId)
{
	bool retVal = false;

	if (m_processMap.contains(serviceId)){
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr){
			process->setProperty("isStopped", true);
			process->terminate();
			process->waitForFinished(2000);
			if (process->isOpen()){
				process->kill();
			}

			retVal = true;
		}
	}

	return retVal;
}


// reimplemented (icomp::CComponentBase)

void CServiceControllerComp::OnComponentCreated()
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(0);

		return;
	}

	imtbase::IObjectCollection::Ids ids = m_serviceCollectionCompPtr->GetElementIds();

	for(const QByteArray& serviceId: ids){
		agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr){
			QByteArray servicePath = serviceInfoPtr->GetServicePath();

			istd::TDelPtr<QProcess> processPtr = new QProcess(this);
			processPtr->setProgram(servicePath);

			QProcess::ProcessState state = processPtr->state();

			if (state == QProcess::Running || serviceInfoPtr->IsAutoStart()){
				StartService(serviceId);
			}
		}
	}
}


void CServiceControllerComp::stateChanged(QProcess::ProcessState newState)
{
	QProcess* senderProcess = dynamic_cast<QProcess*>(sender());

	QList<QByteArray> keys = m_processMap.keys();

	for (const QByteArray& serviceId: keys) {
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr && process == senderProcess){
			{
				istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
				IServiceController::NotifierStatusInfo notifierStatusInfo;
				notifierStatusInfo.serviceId = serviceId;
				notifierStatusInfo.serviceStatus = newState;
				changeSet.SetChangeInfo(IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierStatusInfo));
				istd::CChangeNotifier notifier(this, &changeSet);
			}

			if (newState == QProcess::NotRunning){
				agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (m_serviceCollectionCompPtr.IsValid() && m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
					serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
				}

				bool isStoped = false;

				if (m_processMap.contains(serviceId)){
					isStoped = m_processMap[serviceId]->property("isStopped").toBool();
					m_processMap.take(serviceId)->deleteLater();
				}

				if ((serviceInfoPtr != nullptr && serviceInfoPtr->IsAutoStart() && isStoped != true) || m_restartProcessing.contains(serviceId)){
					StartService(serviceId);

					m_restartProcessing.removeAll(serviceId);
				}
			}

			break;
		}
	}
}


void CServiceControllerComp::updateServiceVersion(const QByteArray& serviceId)
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(0);

		return;
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			QString servicePath = serviceInfoPtr->GetServicePath();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceId];
			pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), nullptr));

			if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")) {
				SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(serviceName), "CServiceControllerComp");
				m_pluginMap.remove(serviceId);

				return;
			}

			if (m_pluginMap.contains(serviceId)){
				const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* connectionCollectionFactoryPtr = nullptr;
				for (int index = 0; index < m_pluginMap[serviceId]->m_plugins.count(); index++){
					imtservice::IConnectionCollectionPlugin* pluginPtr = m_pluginMap[serviceId]->m_plugins[index].pluginPtr;
					if (pluginPtr != nullptr){
						connectionCollectionFactoryPtr = pluginPtr->GetConnectionCollectionFactory();

						break;
					}
				}
				Q_ASSERT(connectionCollectionFactoryPtr != nullptr);
				istd::TDelPtr<imtservice::IConnectionCollection> connectionCollection = connectionCollectionFactoryPtr->CreateInstance();
				if (connectionCollection != nullptr){
					QString serviceVersion = connectionCollection->GetServiceVersion();
					if (serviceInfoPtr->GetServiceVersion() != serviceVersion){
						serviceInfoPtr->SetServiceVersion(serviceVersion);
						m_serviceCollectionCompPtr->SetObjectData(serviceId, *serviceInfoPtr);
					}
				}
			}
		}
	}

}


} // namespace agentinodata


