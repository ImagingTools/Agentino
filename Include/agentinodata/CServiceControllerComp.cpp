#include <agentinodata/CServiceControllerComp.h>


// ACF includes
#include <istd/TDelPtr.h>

// ServiceManager includes
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

	QProcess* process = new QProcess(this);
	m_processMap.insert(serviceId, process);
	connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

	process->setProgram(servicePath);

	QStringList arguments;
	for (QByteArray argument: serviceArguments){
		arguments << QString(argument);
	}

	process->setArguments(arguments);
	process->start();

	return retVal;
}


bool CServiceControllerComp::StopService(const QByteArray& serviceId)
{
	bool retVal = false;

	if (m_processMap.contains(serviceId)){
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr){
			process->terminate();
			process->waitForFinished(4000);
			if (!process->isOpen()){
				process->kill();
			}

			retVal = true;
		}

		m_processMap.take(serviceId)->deleteLater();
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

	for(QByteArray serviceId: ids){
		agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr && serviceInfoPtr->IsAutoStart()){
			StartService(serviceId);
		}
	}
}


void CServiceControllerComp::stateChanged(QProcess::ProcessState newState)
{
	QProcess* senderProcess = dynamic_cast<QProcess*>(sender());

	QList<QByteArray> keys = m_processMap.keys();

	for (QByteArray serviceId: keys) {
		QProcess *process = m_processMap[serviceId];
		if (process != nullptr && process == senderProcess){
			istd::IChangeable::ChangeSet changeSet(istd::IChangeable::CF_ANY);
			IServiceController::NotifierStatusInfo notifierStatusInfo;
			notifierStatusInfo.serviceId = serviceId;
			notifierStatusInfo.serviceStatus = newState;
			changeSet.SetChangeInfo(IServiceController::CN_STATUS_CHANGED, QVariant::fromValue(notifierStatusInfo));
			istd::CChangeNotifier notifier(this, &changeSet);

			if (newState == QProcess::NotRunning){
				agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = nullptr;
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (m_serviceCollectionCompPtr.IsValid() && m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
					serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceDataPtr.GetPtr());
				}

				if (serviceInfoPtr != nullptr && serviceInfoPtr->IsAutoStart()){
					if (m_processMap.contains(serviceId)){
						m_processMap.take(serviceId)->deleteLater();
					}

					StartService(serviceId);
				}
			}

			break;
		}
	}
}


} // namespace agentinodata


