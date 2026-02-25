// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#endif

// Qt includes
#include<QtCore/QFileInfo>

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
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return IServiceStatusInfo::ServiceStatus::SS_UNDEFINED;
	}

	IServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		Q_ASSERT(false);
		return IServiceStatusInfo::ServiceStatus::SS_UNDEFINED;
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();

	IServiceStatusInfo::ServiceStatus retVal = IServiceStatusInfo::SS_NOT_RUNNING;

	QByteArray moduleName = GetModuleName(servicePath);

	if (!moduleName.isEmpty()){
		retVal = IServiceStatusInfo::SS_RUNNING;
	}


	return retVal;
}


bool CServiceControllerComp::StartService(const QByteArray& serviceId)
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return false;
	}

	UpdateServiceVersion(serviceId);

	CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		Q_ASSERT(false);
		return false;
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	QByteArrayList serviceArguments = serviceInfoPtr->GetServiceArguments();

	QStringList arguments;
	for (const QByteArray& argument: serviceArguments){
		if (!argument.isEmpty()){
			arguments << QString(argument);
		}
	}

	QProcess process(this);
	m_activeServiceId = serviceId;
	QByteArray startScriptPath = serviceInfoPtr->GetStartScriptPath();

	if (!startScriptPath.isEmpty()){


		SetupProcess(process, startScriptPath, arguments);

		process.startDetached();

		ServiceProcess serviceProcess;
		m_processMap.insert(serviceId, serviceProcess);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
	}
	else{
		connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(OnReadyReadStandardError()));
		connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(OnReadyReadStandardOutput()));
		if (!m_processMap.contains(serviceId)){
			ServiceProcess serviceProcess;
			serviceProcess.lastStatus = IServiceStatusInfo::SS_STARTING;
			m_processMap.insert(serviceId, serviceProcess);
		}

		SetupProcess(process, servicePath, arguments);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STARTING);
		process.startDetached();

		process.waitForStarted();

		QByteArray moduleName = GetModuleName(servicePath);

		if (!moduleName.isEmpty()){
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);

			m_processMap[serviceId].countOfStarting = 0;
		}
		else{
			m_processMap[serviceId].countOfStarting++;
		}
	}

	QString serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();

	SendInfoMessage(0, QString("Service '%1' started").arg(serviceName), serviceName);

	return true;
}


bool CServiceControllerComp::StopService(const QByteArray& serviceId)
{
	bool retVal = false;


	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return false;
	}

	QString serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();

	agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		Q_ASSERT(false);
		return false;
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
    QByteArray moduleName = GetModuleName(servicePath);

#ifdef WIN32
	if (!moduleName.isEmpty()){
		QByteArray data = "taskkill /f /im " + moduleName;

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);

		system(data.data());

		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

		retVal = true;
	}

#elif defined(Q_OS_LINUX)
	if (!moduleName.isEmpty()){
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);
		QByteArray data = "pkill -9 -f " + moduleName;
		system(data.data());
		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

		retVal = true;
	}
#endif


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
		CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
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

	m_timer.setInterval(5000);
	connect(&m_timer, &QTimer::timeout, this, &CServiceControllerComp::OnTimeout);
	m_timer.start();
}


void CServiceControllerComp::OnReadyReadStandardError()
{
	QProcess* processPtr = dynamic_cast<QProcess*>(sender());

	if (processPtr != nullptr){
		QString serviceName;

		if (m_serviceCollectionCompPtr.IsValid()){
			serviceName = m_serviceCollectionCompPtr->GetElementInfo(m_activeServiceId, imtbase::ICollectionInfo::EIT_NAME).toString();
		}

		QString errorOutput = processPtr->readAllStandardError();

		errorOutput = errorOutput.simplified();

		SendErrorMessage(0, errorOutput, serviceName);
	}
}


void CServiceControllerComp::OnReadyReadStandardOutput()
{
	QProcess* processPtr = dynamic_cast<QProcess*>(sender());

	if (processPtr != nullptr){
		QString serviceName;

		if (m_serviceCollectionCompPtr.IsValid()){
			serviceName = m_serviceCollectionCompPtr->GetElementInfo(m_activeServiceId, imtbase::ICollectionInfo::EIT_NAME).toString();
		}

		QString standardOutput = processPtr->readAllStandardOutput();

		standardOutput = standardOutput.simplified();

		SendInfoMessage(0, standardOutput, serviceName);
	}
}


QByteArray CServiceControllerComp::GetModuleName(QByteArray servicePath) const
{
	QByteArray retVal;

#ifdef Q_OS_WIN
	DWORD processList[10000];
	DWORD bytesNeeded;

	// Get the list of process identifiers.
	if (EnumProcesses(processList, sizeof(processList), &bytesNeeded)){
		// Calculate how many process identifiers were returned.
		DWORD processCount = bytesNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.
		for (DWORD i = 0; i < processCount; i++){
			if (processList[i] != 0){
				DWORD processID = processList[i];

				TCHAR processName[MAX_PATH] = TEXT("<unknown>");

				// Get a handle to the process.
				HANDLE processHandle = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE,
					processID);

				// Get the process name.
				if (nullptr != processHandle){
					HMODULE moduleHandle;
					DWORD byteCount;

					if (EnumProcessModules(processHandle, &moduleHandle, sizeof(moduleHandle),&byteCount)){
						TCHAR processModuleFileName[MAX_PATH];
						if (GetModuleFileNameEx(processHandle, nullptr, processModuleFileName, MAX_PATH)){
							QByteArray modulePath = QString::fromStdWString(processModuleFileName).toUtf8();
							if (servicePath.toLower() == modulePath.toLower()){

								GetModuleBaseName(processHandle, moduleHandle, processName, sizeof(processName) / sizeof(TCHAR));
								retVal = QString::fromStdWString(processName).toUtf8();

								// Release the handle to the process.
								CloseHandle(processHandle);

								return retVal;
							}
						}
					}
				}

				// Release the handle to the process.
				CloseHandle(processHandle);
			}
		}
	}

#elif defined(Q_OS_LINUX)
	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];

	if (!(dir = opendir("/proc"))){
		perror("can't open /proc");
		return "";
	}

	while((ent = readdir(dir)) != NULL){
		/* if endptr is not a null character, the directory is not
		 * entirely numeric, so ignore it */
		long lpid = strtol(ent->d_name, &endptr, 10);
		if (*endptr != '\0'){
			continue;
		}

		/* try to read the exe symlink which always points to the actual executable */
		snprintf(buf, sizeof(buf), "/proc/%ld/exe", lpid);
		ssize_t len = readlink(buf, buf, sizeof(buf) - 1);

		if (len != -1){
			buf[len] = '\0';
			QByteArray modulePath = QString::fromStdString(buf).toUtf8();
			if (servicePath.toLower() == modulePath.toLower()){
				closedir(dir);
				QFileInfo fileInfo(modulePath);
				retVal = fileInfo.fileName().toUtf8();

				return retVal;
			}
		}

	}

	closedir(dir);
#else
	Q_UNUSED(servicePath)
#endif



	return retVal;
}

void CServiceControllerComp::SetupProcess(QProcess& process, const QByteArray& programPath, const QStringList& arguments) const
{
	process.setProgram(programPath);
	process.setArguments(arguments);

	QFileInfo fileInfo(programPath);
	process.setWorkingDirectory(fileInfo.absolutePath());
}


void CServiceControllerComp::UpdateServiceVersion(const QByteArray& serviceId)
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		Q_ASSERT(0);

		return;
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			QString servicePath = serviceInfoPtr->GetServicePath();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceId];
			pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), nullptr));

			if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")){
				SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(qPrintable(serviceName)), "CServiceControllerComp");
				m_pluginMap.remove(serviceId);

				return;
			}

			if (m_pluginMap.contains(serviceId)){
				const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* connectionCollectionFactoryPtr = nullptr;
				for (int index = 0; index < m_pluginMap[serviceId]->m_plugins.count(); index++){
					imtservice::IConnectionCollectionPlugin* pluginPtr = m_pluginMap[serviceId]->m_plugins[index].pluginPtr;
					if (pluginPtr != nullptr){
						if (pluginPtr->GetPluginName() != fileInfo.baseName() + "Settings"){
							continue;
						}

						connectionCollectionFactoryPtr = pluginPtr->GetConnectionCollectionFactory();

						break;
					}
				}

				if (connectionCollectionFactoryPtr != nullptr){
					istd::TUniqueInterfacePtr<imtservice::IConnectionCollection> connectionCollectionPtr = connectionCollectionFactoryPtr->CreateInstance();
					if (connectionCollectionPtr.IsValid()){
						QString serviceVersion = connectionCollectionPtr->GetServiceVersion();
						if (serviceInfoPtr->GetServiceVersion() != serviceVersion){
							serviceInfoPtr->SetServiceVersion(serviceVersion);
							m_serviceCollectionCompPtr->SetObjectData(serviceId, *serviceInfoPtr);
						}
					}
				}
			}
		}
	}
}


void CServiceControllerComp::OnTimeout()
{
	QList<QByteArray> keys = m_processMap.keys();

	for (const QByteArray& serviceId: keys){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr != nullptr){
				QByteArray servicePath = serviceInfoPtr->GetServicePath();
				QByteArray moduleName = GetModuleName(servicePath);
				if (!m_processMap.contains(serviceId)){
					continue;
				}
				IServiceStatusInfo::ServiceStatus lastStatus = m_processMap[serviceId].lastStatus;
				if (moduleName.isEmpty() && lastStatus != IServiceStatusInfo::SS_NOT_RUNNING){
					if (lastStatus == IServiceStatusInfo::SS_STARTING){
						m_processMap[serviceId].countOfStarting++;
						if (m_processMap[serviceId].countOfStarting > 4){
							m_processMap[serviceId].countOfStarting = 0;
							EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
						}
					}
					else{
						if (serviceInfoPtr->IsAutoStart()){
							if (m_processMap[serviceId].countOfStarting > 3){
								EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE);

								continue;
							}

							StartService(serviceId);
						}
						else{
							EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
						}
					}
				}
				else if (!moduleName.isEmpty() && lastStatus != IServiceStatusInfo::SS_RUNNING){
					if (lastStatus == IServiceStatusInfo::SS_STOPPING){
						m_processMap[serviceId].countOfStarting++;
						if (m_processMap[serviceId].countOfStarting > 4){
							m_processMap[serviceId].countOfStarting = 0;
							EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
						}
					}
					else{
						EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
					}
				}
			}
		}
	}
}


void CServiceControllerComp::EmitChangeSignal(const QByteArray& serviceId, IServiceStatusInfo::ServiceStatus serviceStatus)
{
	if (m_processMap.contains(serviceId)){
		m_processMap[serviceId].lastStatus = serviceStatus;
	}

	istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);

	NotifierStatusInfo notifierStatusInfo;
	notifierStatusInfo.serviceId = serviceId;
	notifierStatusInfo.serviceStatus = serviceStatus;

	changeSet.SetChangeInfo(CN_STATUS_CHANGED, QVariant::fromValue(notifierStatusInfo));
	changeSet.SetChangeInfo("serviceid", serviceId);

	istd::CChangeNotifier notifier(this, &changeSet);
}


} // namespace agentinodata


