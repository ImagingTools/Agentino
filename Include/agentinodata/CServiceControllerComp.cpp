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
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#elif defined(Q_OS_MACOS)
#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#endif

// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QThread>

// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>

#ifdef Q_OS_LINUX
#include <limits.h>
#endif


namespace agentinodata
{


namespace
{


// How many OnTimeout ticks (5s) may pass in SS_STARTING without a visible process
// before we conclude start failed. LisaServer / large Debug builds often need >1s
// before QueryFullProcessImageName can see them; path-only matching used to fail
// entirely and the counter then forced SS_NOT_RUNNING while the process was alive.
const int s_maxStartingTicks = 12; // ~60s


} // namespace


// public methods

// reimplemented (agentinodata::IServiceController)

IServiceStatusInfo::ServiceStatus CServiceControllerComp::GetServiceStatus(const QByteArray& serviceId) const
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

	const QByteArray servicePath = serviceInfoPtr->GetServicePath();
	if (IsServiceProcessAlive(serviceId, servicePath)){
		return IServiceStatusInfo::SS_RUNNING;
	}

	// Prefer last transitional status while we still believe a start/stop is in flight.
	if (m_processMap.contains(serviceId)){
		const IServiceStatusInfo::ServiceStatus last = m_processMap[serviceId].lastStatus;
		if (last == IServiceStatusInfo::SS_STARTING || last == IServiceStatusInfo::SS_STOPPING){
			return last;
		}
	}

	return IServiceStatusInfo::SS_NOT_RUNNING;
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

	const QString serviceName =
				m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();

	agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		Q_ASSERT(false);
		return false;
	}

	const QByteArray servicePath = serviceInfoPtr->GetServicePath();

#ifdef Q_OS_WIN
	qint64 pid = 0;
	if (m_processMap.contains(serviceId)){
		pid = m_processMap[serviceId].processId;
	}
	if (pid <= 0 || !IsOsProcessRunning(pid)){
		IsServiceExecutableRunning(servicePath, &pid);
	}

	if (pid > 0 && IsOsProcessRunning(pid)){
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);

		// Prefer PID kill so we do not taskkill every process with the same image name.
		const QByteArray killCmd = "taskkill /f /pid " + QByteArray::number(pid);
		system(killCmd.constData());

		if (m_processMap.contains(serviceId)){
			m_processMap[serviceId].processId = 0;
			m_processMap[serviceId].countOfStarting = 0;
		}

		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
		retVal = true;
	}
	else{
		// Fallback: image-name kill (legacy behaviour).
		const QByteArray moduleName = GetModuleName(servicePath);
		if (!moduleName.isEmpty()){
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);
			const QByteArray data = "taskkill /f /im " + moduleName;
			system(data.constData());
			if (m_processMap.contains(serviceId)){
				m_processMap[serviceId].processId = 0;
				m_processMap[serviceId].countOfStarting = 0;
			}
			SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
			retVal = true;
		}
	}

#elif defined(Q_OS_LINUX)
	qint64 pid = 0;
	if (m_processMap.contains(serviceId)){
		pid = m_processMap[serviceId].processId;
	}
	if (pid <= 0 || !IsOsProcessRunning(pid)){
		IsServiceExecutableRunning(servicePath, &pid);
	}

	if (pid > 0 && IsOsProcessRunning(pid)){
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);
		::kill(static_cast<pid_t>(pid), SIGKILL);
		if (m_processMap.contains(serviceId)){
			m_processMap[serviceId].processId = 0;
			m_processMap[serviceId].countOfStarting = 0;
		}
		SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
		retVal = true;
	}
	else{
		const QByteArray moduleName = GetModuleName(servicePath);
		if (!moduleName.isEmpty()){
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);
			const QByteArray data = "pkill -9 -f " + moduleName;
			system(data.constData());
			if (m_processMap.contains(serviceId)){
				m_processMap[serviceId].processId = 0;
				m_processMap[serviceId].countOfStarting = 0;
			}
			SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
			retVal = true;
		}
	}

#elif defined(Q_OS_MACOS)
	QFileInfo serviceInfo(QString::fromUtf8(servicePath));
	QString targetPath = serviceInfo.canonicalFilePath();
	if (targetPath.isEmpty()){
		targetPath = QDir::cleanPath(serviceInfo.absoluteFilePath());
	}

	std::vector<pid_t> pids = GetPidsForPath(targetPath);
	if (m_processMap.contains(serviceId) && m_processMap[serviceId].processId > 0){
		pids.push_back(static_cast<pid_t>(m_processMap[serviceId].processId));
	}
	if (pids.empty()){
		return false;
	}

	bool stopRequested = false;
	for (pid_t pid : pids){
		if (::kill(pid, SIGTERM) == 0){
			stopRequested = true;
		}
	}

	if (stopRequested){
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STOPPING);

		int retries = 10;
		std::vector<pid_t> remainingPids;
		while (retries > 0){
			QThread::msleep(150);
			remainingPids = GetPidsForPath(targetPath);
			if (remainingPids.empty()){
				break;
			}
			--retries;
		}

		for (pid_t pid : remainingPids){
			::kill(pid, SIGKILL);
		}

		std::vector<pid_t> finalPids = GetPidsForPath(targetPath);
		if (finalPids.empty()){
			if (m_processMap.contains(serviceId)){
				m_processMap[serviceId].processId = 0;
				m_processMap[serviceId].countOfStarting = 0;
			}
			SendInfoMessage(0, QString("Service '%1' stopped").arg(serviceName), serviceName);
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
			retVal = true;
		}
		else{
			SendErrorMessage(
						0,
						QString("Failed to stop service '%1': some processes could not be terminated").arg(serviceName),
						serviceName);
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

	for (const QByteArray& serviceId: ids){
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
				SendErrorMessage(
							0,
							QString("Failed to stop service '%1' during shutdown").arg(QString(serviceId)),
							"CServiceControllerComp");
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


// private methods

QString CServiceControllerComp::NormalizeExecutablePath(const QByteArray& servicePath)
{
	if (servicePath.isEmpty()){
		return QString();
	}

	const QFileInfo fileInfo(QString::fromUtf8(servicePath));
	QString path = fileInfo.canonicalFilePath();
	if (path.isEmpty()){
		path = QDir::cleanPath(fileInfo.absoluteFilePath());
	}

	return QDir::toNativeSeparators(path).toLower();
}


bool CServiceControllerComp::IsOsProcessRunning(qint64 pid)
{
	if (pid <= 0){
		return false;
	}

#ifdef Q_OS_WIN
	HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
	if (processHandle == nullptr){
		return false;
	}

	DWORD exitCode = 0;
	const BOOL ok = GetExitCodeProcess(processHandle, &exitCode);
	CloseHandle(processHandle);

	return ok && exitCode == STILL_ACTIVE;

#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
	// kill(pid, 0) succeeds if the process exists and we may signal it.
	if (::kill(static_cast<pid_t>(pid), 0) == 0){
		return true;
	}

	return errno == EPERM;

#else
	Q_UNUSED(pid);

	return false;
#endif
}


bool CServiceControllerComp::IsServiceExecutableRunning(const QByteArray& servicePath, qint64* outPid) const
{
	if (outPid != nullptr){
		*outPid = 0;
	}

	const QString targetPath = NormalizeExecutablePath(servicePath);
	if (targetPath.isEmpty()){
		return false;
	}

#ifdef Q_OS_WIN
	DWORD processList[10000];
	DWORD bytesNeeded = 0;
	if (!EnumProcesses(processList, sizeof(processList), &bytesNeeded)){
		return false;
	}

	const DWORD processCount = bytesNeeded / sizeof(DWORD);
	for (DWORD i = 0; i < processCount; ++i){
		const DWORD processId = processList[i];
		if (processId == 0){
			continue;
		}

		// LIMITED_INFORMATION is enough for QueryFullProcessImageName and works for more
		// processes than PROCESS_QUERY_INFORMATION | PROCESS_VM_READ (old EnumProcessModules path).
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
		if (processHandle == nullptr){
			continue;
		}

		WCHAR imagePath[MAX_PATH * 4];
		DWORD imagePathSize = MAX_PATH * 4;
		const BOOL gotPath = QueryFullProcessImageNameW(processHandle, 0, imagePath, &imagePathSize);
		CloseHandle(processHandle);

		if (!gotPath){
			continue;
		}

		const QString modulePath = NormalizeExecutablePath(QString::fromWCharArray(imagePath, static_cast<int>(imagePathSize)).toUtf8());
		if (modulePath == targetPath){
			if (outPid != nullptr){
				*outPid = static_cast<qint64>(processId);
			}

			return true;
		}
	}

	return false;

#elif defined(Q_OS_LINUX)
	DIR* dir = opendir("/proc");
	if (dir == nullptr){
		return false;
	}

	struct dirent* ent = nullptr;
	while ((ent = readdir(dir)) != nullptr){
		char* endptr = nullptr;
		const long lpid = strtol(ent->d_name, &endptr, 10);
		if (endptr == nullptr || *endptr != '\0' || lpid <= 0){
			continue;
		}

		// Prefer /proc/<pid>/exe symlink (canonical path) over cmdline.
		char linkBuf[PATH_MAX];
		char exeBuf[PATH_MAX];
		snprintf(linkBuf, sizeof(linkBuf), "/proc/%ld/exe", lpid);
		const ssize_t len = readlink(linkBuf, exeBuf, sizeof(exeBuf) - 1);
		if (len <= 0){
			continue;
		}
		exeBuf[len] = '\0';

		const QString modulePath = NormalizeExecutablePath(QByteArray(exeBuf, static_cast<int>(len)));
		if (modulePath == targetPath){
			closedir(dir);
			if (outPid != nullptr){
				*outPid = static_cast<qint64>(lpid);
			}

			return true;
		}
	}

	closedir(dir);

	return false;

#elif defined(Q_OS_MACOS)
	std::vector<pid_t> pids = GetPidsForPath(targetPath);
	if (pids.empty()){
		// GetPidsForPath expects canonical path; retry with our normalized form.
		pids = GetPidsForPath(QFileInfo(QString::fromUtf8(servicePath)).canonicalFilePath());
	}
	if (!pids.empty()){
		if (outPid != nullptr){
			*outPid = static_cast<qint64>(pids.front());
		}

		return true;
	}

	return false;

#else
	Q_UNUSED(servicePath);
	Q_UNUSED(outPid);

	return false;
#endif
}


bool CServiceControllerComp::IsServiceProcessAlive(const QByteArray& serviceId, const QByteArray& servicePath) const
{
	// 1) Tracked PID from startDetached — reliable even when image path strings disagree.
	if (m_processMap.contains(serviceId)){
		const qint64 pid = m_processMap[serviceId].processId;
		if (pid > 0 && IsOsProcessRunning(pid)){
			return true;
		}
	}

	// 2) Full-path scan of the process table (normalized).
	qint64 foundPid = 0;
	if (IsServiceExecutableRunning(servicePath, &foundPid)){
		if (foundPid > 0 && m_processMap.contains(serviceId)){
			// Refresh cached PID for stop / later polls.
			m_processMap[serviceId].processId = foundPid;
		}

		return true;
	}

	return false;
}


QByteArray CServiceControllerComp::GetModuleName(const QByteArray& servicePath) const
{
	qint64 pid = 0;
	if (!IsServiceExecutableRunning(servicePath, &pid)){
		return QByteArray();
	}

	return QFileInfo(QString::fromUtf8(servicePath)).fileName().toUtf8();
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

	const QByteArray servicePath = serviceInfoPtr->GetServicePath();
	const QByteArrayList serviceArguments = serviceInfoPtr->GetServiceArguments();

	QStringList arguments;
	for (const QByteArray& argument: serviceArguments){
		if (!argument.isEmpty()){
			arguments << QString::fromUtf8(argument);
		}
	}

	m_activeServiceId = serviceId;
	const QByteArray startScriptPath = serviceInfoPtr->GetStartScriptPath();

	if (!m_processMap.contains(serviceId)){
		m_processMap.insert(serviceId, ServiceProcess());
	}

	const QFileInfo programInfo(QString::fromUtf8(startScriptPath.isEmpty() ? servicePath : startScriptPath));
	const QString program = programInfo.absoluteFilePath();
	const QString workingDirectory = programInfo.absolutePath();

	if (!startScriptPath.isEmpty()){
		qint64 pid = 0;
		const bool started = QProcess::startDetached(program, arguments, workingDirectory, &pid);
		if (!started){
			SendErrorMessage(
						0,
						QString("Unable to start service script '%1'").arg(program),
						"CServiceControllerComp");
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

			return false;
		}

		m_processMap[serviceId].processId = pid;
		m_processMap[serviceId].countOfStarting = 0;
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
	}
	else if (startRequired){
		// Already running (e.g. previous start not tracked) — report RUNNING, do not spawn again.
		qint64 existingPid = 0;
		if (IsServiceExecutableRunning(servicePath, &existingPid)){
			m_processMap[serviceId].processId = existingPid;
			m_processMap[serviceId].countOfStarting = 0;
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);

			const QString serviceName =
						m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
			SendInfoMessage(0, QString("Service '%1' already running").arg(serviceName), serviceName);

			return true;
		}

		m_processMap[serviceId].countOfStarting = 0;
		m_processMap[serviceId].processId = 0;
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_STARTING);

		// No QObject parent: StartService often runs on a GQL worker thread; parenting to
		// this (main-thread affinity) is illegal and was never required for startDetached.
		qint64 pid = 0;
		const bool started = QProcess::startDetached(program, arguments, workingDirectory, &pid);
		if (!started){
			SendErrorMessage(
						0,
						QString("Unable to start service executable '%1'").arg(program),
						"CServiceControllerComp");
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

			return false;
		}

		if (pid > 0){
			m_processMap[serviceId].processId = pid;
		}

		// Brief settle: large Debug binaries (e.g. LisaServer) may not answer path queries
		// for a short time even though the PID is already valid.
		if (pid > 0 && IsOsProcessRunning(pid)){
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
			m_processMap[serviceId].countOfStarting = 0;
		}
		else{
			// Leave SS_STARTING; OnTimeout will promote to RUNNING once the process is visible,
			// or to NOT_RUNNING after s_maxStartingTicks if it never appears.
			qint64 foundPid = 0;
			if (IsServiceExecutableRunning(servicePath, &foundPid)){
				m_processMap[serviceId].processId = foundPid;
				m_processMap[serviceId].countOfStarting = 0;
				EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
			}
		}
	}
	else{
		m_processMap[serviceId].processId = 0;
		m_processMap[serviceId].countOfStarting = 0;
		EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
	}

	const QString serviceName =
				m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::ICollectionInfo::EIT_NAME).toString();
	if (startRequired){
		SendInfoMessage(0, QString("Service '%1' started").arg(serviceName), serviceName);
	}

	return true;
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
			const QByteArray serviceName =
						m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			const QString servicePath = serviceInfoPtr->GetServicePath();

			const QFileInfo fileInfo(servicePath);
			const QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceId];
			pluginManagerPtr.SetPtr(new PluginManager(
						IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings),
						IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings),
						nullptr));

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
					istd::TUniqueInterfacePtr<imtservice::IConnectionCollection> connectionCollectionPtr =
								connectionCollectionFactoryPtr->CreateInstance();
					if (connectionCollectionPtr.IsValid()){
						const QString serviceVersion = connectionCollectionPtr->GetServiceVersion();
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
	const QList<QByteArray> keys = m_processMap.keys();

	for (const QByteArray& serviceId: keys){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			continue;
		}

		IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		if (!m_processMap.contains(serviceId)){
			continue;
		}

		const QByteArray servicePath = serviceInfoPtr->GetServicePath();
		const bool alive = IsServiceProcessAlive(serviceId, servicePath);
		const IServiceStatusInfo::ServiceStatus lastStatus = m_processMap[serviceId].lastStatus;

		if (alive){
			if (lastStatus != IServiceStatusInfo::SS_RUNNING){
				if (lastStatus == IServiceStatusInfo::SS_STOPPING){
					// Process still present while stopping — wait a few ticks, then accept RUNNING.
					m_processMap[serviceId].countOfStarting++;
					if (m_processMap[serviceId].countOfStarting > 4){
						m_processMap[serviceId].countOfStarting = 0;
						EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
					}
				}
				else{
					m_processMap[serviceId].countOfStarting = 0;
					EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING);
				}
			}

			continue;
		}

		// Not alive
		if (lastStatus == IServiceStatusInfo::SS_NOT_RUNNING){
			continue;
		}

		if (lastStatus == IServiceStatusInfo::SS_STARTING){
			m_processMap[serviceId].countOfStarting++;
			if (m_processMap[serviceId].countOfStarting > s_maxStartingTicks){
				m_processMap[serviceId].countOfStarting = 0;
				m_processMap[serviceId].processId = 0;
				EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
			}

			continue;
		}

		if (lastStatus == IServiceStatusInfo::SS_STOPPING){
			m_processMap[serviceId].processId = 0;
			m_processMap[serviceId].countOfStarting = 0;
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);

			continue;
		}

		// Was RUNNING (or similar) and process disappeared.
		if (serviceInfoPtr->IsAutoStart()){
			if (m_processMap[serviceId].countOfStarting > 3){
				EmitChangeSignal(serviceId, IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE);

				continue;
			}

			StartService(serviceId);
		}
		else{
			m_processMap[serviceId].processId = 0;
			EmitChangeSignal(serviceId, IServiceStatusInfo::SS_NOT_RUNNING);
		}
	}
}


void CServiceControllerComp::EmitChangeSignal(const QByteArray& serviceId, IServiceStatusInfo::ServiceStatus serviceStatus)
{
	// Synchronous notify on the caller thread. PublishData is safe off-main
	// (mutex + QueuedConnection to the socket thread).
	if (m_processMap.contains(serviceId)){
		if (m_processMap[serviceId].lastStatus == serviceStatus){
			return;
		}
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

std::vector<pid_t> CServiceControllerComp::GetPidsForPath(const QString& targetPath)
{
	std::vector<pid_t> matchingPids;
	if (targetPath.isEmpty()){
		return matchingPids;
	}

	int bytesNeeded = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
	if (bytesNeeded <= 0){
		return matchingPids;
	}

	size_t pidCount = (static_cast<size_t>(bytesNeeded) / sizeof(pid_t)) + 64;
	std::vector<pid_t> processList(pidCount);

	int bytesReturned = proc_listpids(
				PROC_ALL_PIDS,
				0,
				processList.data(),
				static_cast<int>(processList.size() * sizeof(pid_t)));
	if (bytesReturned <= 0){
		return matchingPids;
	}

	size_t actualCount = static_cast<size_t>(bytesReturned) / sizeof(pid_t);
	if (actualCount > processList.size()){
		actualCount = processList.size();
	}

	const QString normalizedTarget = QDir::toNativeSeparators(targetPath).toLower();

	for (size_t i = 0; i < actualCount; ++i){
		pid_t pid = processList[i];
		if (pid <= 0){
			continue;
		}

		char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
		int pathLen = proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));
		if (pathLen > 0){
			QString currentPath = QDir::toNativeSeparators(QString::fromUtf8(pathBuffer, pathLen)).toLower();
			if (currentPath == normalizedTarget){
				matchingPids.push_back(pid);
			}
		}
	}

	return matchingPids;
}

#endif


} // namespace agentinodata
