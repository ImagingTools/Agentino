#include <agentinodata/CServiceControllerComp.h>


// ACF includes
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceController)

IServiceStatusInfo::ServiceStatus  CServiceControllerComp::GetServiceStatus(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus retVal = IServiceStatusInfo::SS_NOT_RUNNING;

	if (m_processMap.contains(serviceId)){
		QProcess::ProcessState processState = m_processMap[serviceId]->state();
		switch (processState) {
		case QProcess::Running:
				retVal = IServiceStatusInfo::SS_RUNNING;
			break;
		case QProcess::NotRunning:
				retVal = IServiceStatusInfo::SS_NOT_RUNNING;
			break;
		case QProcess::Starting:
				retVal = IServiceStatusInfo::SS_STARTING;
			break;
		}
	}

	return retVal;
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
		connect(process, SIGNAL(readyReadStandardError()), this, SLOT(OnReadyReadStandardError()));
		connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(OnReadyReadStandardOutput()));

		process->setProgram(servicePath);
		QFileInfo fileInfo(servicePath);
		process->setWorkingDirectory(fileInfo.absolutePath());
		QStringList arguments;
		for (const QByteArray& argument: serviceArguments){
			if (!argument.isEmpty()) {
				arguments << QString(argument);
			}
		}
		if (!arguments.isEmpty()) {
			process->setArguments(arguments);
		}
	}

	QString serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();

	SendInfoMessage(0, QString("Start service: %1").arg(serviceName), serviceName);

	process->start();


	return true;
}


bool CServiceControllerComp::StopService(const QByteArray& serviceId)
{
	bool retVal = false;

	QString serviceName;

	if (m_serviceCollectionCompPtr.IsValid()){
		serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
	}

	SendInfoMessage(0, QString("Stop service: %1").arg(serviceName), serviceName);

	if (m_processMap.contains(serviceId)){
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr){
			process->setProperty("isStopped", true);
			process->close();
			process->waitForFinished(2000);
			if (process->isOpen()){
				process->kill();
			}

			retVal = true;
		}
	}

	return retVal;
}


// reimplemented (istd::ILogger)
void CServiceControllerComp::DecorateMessage(
			istd::IInformationProvider::InformationCategory category,
			int id,
			int flags,
			QString& message,
			QString& messageSource) const
{
	BaseClass2::DecorateMessage(category, id, flags, message, messageSource);
}


// reimplemented (icomp::CComponentBase)

void CServiceControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

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
	IServiceStatusInfo::ServiceStatus serviceStatus = IServiceStatusInfo::SS_NOT_RUNNING;
	switch (newState) {
	case QProcess::Running:
		serviceStatus = IServiceStatusInfo::SS_RUNNING;
		break;
	case QProcess::NotRunning:
		serviceStatus = IServiceStatusInfo::SS_NOT_RUNNING;
		break;
	case QProcess::Starting:
		serviceStatus = IServiceStatusInfo::SS_STARTING;
		break;
	}

	QProcess* senderProcess = dynamic_cast<QProcess*>(sender());

	QList<QByteArray> keys = m_processMap.keys();

	for (const QByteArray& serviceId: keys) {
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr && process == senderProcess){
			{
				istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
				IServiceController::NotifierStatusInfo notifierStatusInfo;
				notifierStatusInfo.serviceId = serviceId;
				notifierStatusInfo.serviceStatus = serviceStatus;
				changeSet.SetChangeInfo(IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierStatusInfo));
				istd::CChangeNotifier notifier(this, &changeSet);
				QString serviceName;
				if (m_serviceCollectionCompPtr.IsValid()){
					serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
				}
				SendInfoMessage(0, "Service state changed: " + IServiceStatusInfo::ToString(serviceStatus), serviceName);
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


void CServiceControllerComp::OnReadyReadStandardError()
{
	QProcess* processPtr = dynamic_cast<QProcess*>(sender());
	QByteArray serviceId = "Unknown service";
	for (QByteArray id: m_processMap.keys()){
		if (m_processMap[id] == processPtr){
			serviceId = id;
		}
	}

	if (processPtr != nullptr){
		QString serviceName;

		if (m_serviceCollectionCompPtr.IsValid()){
			serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
		}

		QString errorOutput = processPtr->readAllStandardError();

		errorOutput = errorOutput.simplified();

		SendErrorMessage(0, errorOutput, serviceName);
	}
}


void CServiceControllerComp::OnReadyReadStandardOutput()
{
	QProcess* processPtr = dynamic_cast<QProcess*>(sender());
	QByteArray serviceId = "Unknown service";
	for (QByteArray id: m_processMap.keys()){
		if (m_processMap[id] == processPtr){
			serviceId = id;
		}
	}

	if (processPtr != nullptr){
		QString serviceName;

		if (m_serviceCollectionCompPtr.IsValid()){
			serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
		}

		QString standardOutput = processPtr->readAllStandardOutput();

		standardOutput = standardOutput.simplified();

		SendInfoMessage(0, standardOutput, serviceName);
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


