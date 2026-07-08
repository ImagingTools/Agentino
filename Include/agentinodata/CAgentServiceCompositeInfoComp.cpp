// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CAgentServiceCompositeInfoComp.h>


// Qt includes
#include <QtCore/QSysInfo>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceCompositeInfo)

QByteArray CAgentServiceCompositeInfoComp::GetServiceId(const QUrl& url) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	return FindServiceIdByUrl(*m_serviceCollectionCompPtr, url);
}


QByteArray CAgentServiceCompositeInfoComp::GetServiceId(const QByteArray& dependantServiceConnectionId) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	return FindServiceIdByDependantConnectionId(*m_serviceCollectionCompPtr, dependantServiceConnectionId);
}


IServiceStatusInfo::ServiceStatus CAgentServiceCompositeInfoComp::GetServiceStatus(const QByteArray& serviceId) const
{
	if (serviceId.isEmpty() || !m_serviceStatusProviderCompPtr.IsValid()){
		// Services of other agents cannot be resolved locally - the status is unknown
		return IServiceStatusInfo::SS_UNDEFINED;
	}

	return m_serviceStatusProviderCompPtr->GetServiceStatus(serviceId);
}


IServiceCompositeInfo::StateOfRequiredServices CAgentServiceCompositeInfoComp::GetStateOfRequiredServices(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus serviceStatus = GetServiceStatus(serviceId);
	if (serviceStatus == IServiceStatusInfo::SS_UNDEFINED || serviceStatus == IServiceStatusInfo::SS_NOT_RUNNING){
		return SORS_UNDEFINED;
	}

	if (!m_serviceCollectionCompPtr.IsValid()){
		return SORS_UNDEFINED;
	}

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			return CalculateStateOfRequiredServices(*serviceInfoPtr);
		}
	}

	return SORS_UNDEFINED;
}


IServiceCompositeInfo::Ids CAgentServiceCompositeInfoComp::GetDependencyServices(const QByteArray& serviceId) const
{
	Ids retVal;

	if (!m_serviceCollectionCompPtr.IsValid()){
		return retVal;
	}

	CollectDependencyServices(*m_serviceCollectionCompPtr, serviceId, retVal);

	return retVal;
}


QString CAgentServiceCompositeInfoComp::GetServiceName(const QByteArray& serviceId) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return QString();
	}

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			return serviceInfoPtr->GetServiceName();
		}
	}

	return QString();
}


QString CAgentServiceCompositeInfoComp::GetServiceAgentName(const QByteArray& serviceId) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		return QString();
	}

	imtbase::ICollectionInfo::Ids serviceElementIds = m_serviceCollectionCompPtr->GetElementIds();
	if (serviceElementIds.contains(serviceId)){
		return QSysInfo::machineHostName();
	}

	return QString();
}


} // namespace agentinodata
