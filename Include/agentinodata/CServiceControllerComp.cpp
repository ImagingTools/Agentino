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
#elif defined(Q_OS_MACOS)
#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#include <vector>
#endif

// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>

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
	return SetupService(serviceId, true);
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

#ifdef WIN32
	QByteArray moduleName = GetModuleName(servicePath);
	if (!moduleName.isEmpty()){
		QByteArray data = "taskkill /f /im " + moduleName;

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);

		system(data.data());

		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

		retVal = true;
	}

#elif defined(Q_OS_LINUX)
	QByteArray moduleName = GetModuleName(servicePath);
	if (!moduleName.isEmpty()){
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);
		QByteArray data = "pkill -9 -f " + moduleName;
		system(data.data());
		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);

		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

		retVal = true;
	}
#elif defined(Q_OS_MACOS)
	QFileInfo serviceInfo(QString::fromUtf8(servicePath));
	QString targetPath = serviceInfo.canonicalFilePath();
	if (targetPath.isEmpty()) {
		targetPath = QDir::cleanPath(serviceInfo.absoluteFilePath());
	}

	std::vector<pid_t> pids = GetPidsForPath(targetPath);
	if (pids.empty()) {
		// Service is not running
		return false;
	}

	bool stopRequested = false;

	// Try stopping the processes gracefully first via SIGTERM
	for (pid_t pid : pids) {
		if (::kill(pid, SIGTERM) == 0) {
			stopRequested = true;
		}
	}

	if (stopRequested) {
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);

		// active-wait fallback: verification & SIGKILL override
		// Wait up to 1.5 seconds (in 150ms increments) for the processes to exit
		int retries = 10;
		std::vector<pid_t> remainingPids;
		while (retries > 0) {
			QThread::msleep(150);
			remainingPids = GetPidsForPath(targetPath);
			if (remainingPids.empty()) {
				break;
			}
			--retries;
		}

		// If some PIDs are still lingering, forcibly terminate them via SIGKILL
		for (pid_t pid : remainingPids) {
			::kill(pid, SIGKILL);
		}

		// Verify all processes are actually gone after SIGKILL
		std::vector<pid_t> finalPids = GetPidsForPath(targetPath);
		if (finalPids.empty()) {
			SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
			retVal = true;
		} else {
			SendErrorMessage(0, QString("Failed to stop service '%1': some processes could not be terminated").arg(serviceName), serviceName);
		}
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
			IServiceStatusInfo::ServiceStatus status = GetServiceStatus(serviceId);

			if (status == IServiceStatusInfo::SS_NOT_RUNNING && serviceInfoPtr->IsAutoStart()){
				SetupService(serviceId, true);
			}
			else{
				SetupService(serviceId, false);
			}
		}
	}

	m_timer.setInterval(5000);
	connect(&m_timer, &QTimer::timeout, this, &CServiceControllerComp::OnTimeout);
	m_timer.start();
}


void CServiceControllerComp::OnComponentDestroyed()
{
	m_timer.stop();

	QList<QByteArray> serviceIds = m_processMap.keys();
	for (const QByteArray& serviceId : serviceIds){
		if (m_processMap[serviceId].lastStatus == IServiceStatusInfo::SS_RUNNING
			|| m_processMap[serviceId].lastStatus == IServiceStatusInfo::SS_STARTING){
			if (!StopService(serviceId)){
				SendErrorMessage(0, QString("Failed to stop service '%1' during shutdown").arg(QString(serviceId)), "CServiceControllerComp");
			}
		}
	}

	m_processMap.clear();
	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
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

		/* try to open the cmdline file */
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");

		if (fp){
			if (fgets(buf, sizeof(buf), fp) != NULL){
				/* check the first token in the file, the program name */
				QByteArray modulePath = QString::fromStdString(buf).toUtf8();
				if (servicePath.toLower() == modulePath.toLower()){
					closedir(dir);
					char* first = strtok(buf, " ");
					retVal = QString::fromStdString(first).toUtf8();
					QFileInfo fileInfo(retVal);
					retVal = fileInfo.fileName().toUtf8();

					return retVal;
				}
			}
			fclose(fp);
		}

	}

	closedir(dir);

#elif defined(Q_OS_MACOS)
	QFileInfo serviceInfo(QString::fromUtf8(servicePath));
	QString targetPath = serviceInfo.canonicalFilePath();
	if (targetPath.isEmpty()) {
		targetPath = QDir::cleanPath(serviceInfo.absoluteFilePath());
	}

	std::vector<pid_t> pids = GetPidsForPath(targetPath);

	if (!pids.empty()) {
		return QFileInfo(targetPath).fileName().toUtf8();
	}

#else

	Q_UNUSED(servicePath)

#endif



	return retVal;
}


bool CServiceControllerComp::SetupService(const QByteArray& serviceId, bool startRequired)
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
			// serviceProcess.lastStatus = IServiceStatusInfo::SS_STARTING;
			m_processMap.insert(serviceId, serviceProcess);
		}

		SetupProcess(process, servicePath, arguments);

		if (startRequired){
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
		else{
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
		}
	}

	QString serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();

	SendInfoMessage(0, QString("Service '%1' started").arg(serviceName), serviceName);

	return true;
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

#ifdef Q_OS_MACOS

std::vector<pid_t> CServiceControllerComp::GetPidsForPath(const QString &targetPath)
{
	std::vector<pid_t> matchingPids;
	if (targetPath.isEmpty()) {
		return matchingPids;
	}

	int bytesNeeded = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
	if (bytesNeeded <= 0) {
		return matchingPids;
	}

	// Allocate with a safety buffer to absorb newly spawned processes
	size_t pidCount = (static_cast<size_t>(bytesNeeded) / sizeof(pid_t)) + 64;
	std::vector<pid_t> processList(pidCount);

	int bytesReturned = proc_listpids(PROC_ALL_PIDS, 0, processList.data(), static_cast<int>(processList.size() * sizeof(pid_t)));
	if (bytesReturned <= 0) {
		return matchingPids;
	}

	size_t actualCount = static_cast<size_t>(bytesReturned) / sizeof(pid_t);
	if (actualCount > processList.size()) {
		actualCount = processList.size();
	}

	for (size_t i = 0; i < actualCount; ++i) {
		pid_t pid = processList[i];
		if (pid <= 0) {
			continue;
		}

		char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
		int pathLen = proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));
		if (pathLen > 0) {
			// Using explicit pathLen to avoid redundant strlen parsing inside QString
			QString currentPath = QString::fromUtf8(pathBuffer, pathLen);
			if (currentPath.compare(targetPath, Qt::CaseInsensitive) == 0) {
				matchingPids.push_back(pid);
			}
		}
	}

	return matchingPids;
}

#endif


} // namespace agentinodata


