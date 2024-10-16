#include <agentinodata/CServiceCompositeInfoComp.h>


// Qt includes
#include <QtCore/QDebug>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>

// Qt includes
#include <QtCore/QDebug>


namespace agentinodata
{


// public methods

// reimplemented (agentinodata::IServiceCompositeInfo)

QByteArray CServiceCompositeInfoComp::GetServiceId(const QUrl& url) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}
			// Get Services
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}
			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}
					// Get Connections
					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}
					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						qDebug() << "connectionElementId" << connectionElementId;
						imtbase::IObjectCollection::DataPtr connectionDataPtr;
						if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
							imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
							if (connectionParamPtr != nullptr){
								if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
									if (connectionParamPtr->GetUrl() == url){
										return serviceElementId;
									}

									QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
									for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
										if (incomingConnection.url == url){
											return serviceElementId;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return QByteArray();
}


QByteArray CServiceCompositeInfoComp::GetServiceId(const QByteArray& dependantServiceConnectionId) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArray();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}
			// Get Services
			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}
			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}
					// Get Connections
					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}
					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						qDebug() << "connectionElementId" << connectionElementId;
						if (connectionElementId == dependantServiceConnectionId){
							return serviceElementId;
						}
					}
				}
			}
		}
	}

	return QByteArray();
}


IServiceStatusInfo::ServiceStatus CServiceCompositeInfoComp::GetServiceStatus(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus retVal = IServiceStatusInfo::SS_UNDEFINED;
	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr serviceStatusDataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, serviceStatusDataPtr)){
			IServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<IServiceStatusInfo*>(serviceStatusDataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				retVal = serviceStatusInfoPtr->GetServiceStatus();
			}
		}
	}

	return retVal;
}


IServiceCompositeInfo::StateOfRequiredServices CServiceCompositeInfoComp::GetStateOfRequiredServices(const QByteArray& serviceId) const
{
	IServiceStatusInfo::ServiceStatus serviceStatus = GetServiceStatus(serviceId);
	if (serviceStatus == IServiceStatusInfo::SS_UNDEFINED || serviceStatus == IServiceStatusInfo::SS_NOT_RUNNING){
		return IServiceCompositeInfo::SORS_UNDEFINED;
	}

	IServiceCompositeInfo::StateOfRequiredServices retVal = IServiceCompositeInfo::SORS_RUNNING;

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
							agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr == nullptr){
								continue;
							}
							// Get Connections
							imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
							if (connectionCollectionPtr != nullptr){
								imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
								for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
									imtbase::IObjectCollection::DataPtr connectionDataPtr;
									if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
										imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
										if (connectionLinkParamPtr != nullptr){
											QByteArray dependantServiceId =  GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
											IServiceStatusInfo::ServiceStatus dependantServiceStatus = GetServiceStatus(dependantServiceId);
											if (dependantServiceStatus == IServiceStatusInfo::SS_UNDEFINED){
												return IServiceCompositeInfo::SORS_UNDEFINED;
											}
											else if (dependantServiceStatus == IServiceStatusInfo::SS_NOT_RUNNING){
												retVal = IServiceCompositeInfo::SORS_NOT_RUNNING;
											}
											else if (dependantServiceStatus == IServiceStatusInfo::SS_RUNNING && retVal != IServiceCompositeInfo::SORS_NOT_RUNNING){
												retVal = IServiceCompositeInfo::SORS_RUNNING;
											}
										}
									}
								}
							}
						}

						break;
					}
				}
			}
		}
	}

	return retVal;
}


QByteArrayList CServiceCompositeInfoComp::GetDependencyServices(const QByteArray& serviceId) const
{
	QByteArrayList retVal;

	if (!m_agentCollectionCompPtr.IsValid()){
		return QByteArrayList();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					for (QByteArray serviceElementId:serviceElementIds){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
							agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr == nullptr){
								continue;
							}
							// Get Connections
							imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
							if (connectionCollectionPtr != nullptr){
								imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
								for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
									imtbase::IObjectCollection::DataPtr connectionDataPtr;
									if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
										imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
										if (connectionLinkParamPtr != nullptr){
											QByteArray dependantServiceId =  GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
											if (dependantServiceId == serviceId){
												retVal << serviceElementId;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return retVal;
}


QString CServiceCompositeInfoComp::GetServiceName(const QByteArray& serviceId) const
{
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						QString serviceName = serviceCollectionPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toString();

						return serviceName;
					}
				}
			}
		}
	}

	return QString();
}


QString CServiceCompositeInfoComp::GetServiceAgentName(const QByteArray& serviceId) const
{
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						QString agentName = m_agentCollectionCompPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();

						return agentName;
					}
				}
			}
		}
	}

	return QString();
}


} // namespace agentinodata


