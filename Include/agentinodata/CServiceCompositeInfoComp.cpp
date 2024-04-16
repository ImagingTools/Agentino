#include <agentinodata/CServiceCompositeInfoComp.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/IAgentInfo.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>



namespace agentinodata
{

//public implemented
// reimplemented (agentinodata::IServiceCompositeInfo)


QByteArray CServiceCompositeInfoComp::GetServiceId(const QUrl& url, const QString& connectionServiceTypeName) const
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
									if (connectionParamPtr->GetUrl() == url/* && connectionServiceName == serviceName*/){
										return serviceElementId;
									}

									QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
									for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
										if (incomingConnection.url == url/* && connectionServiceName == serviceName*/){
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


QString CServiceCompositeInfoComp::GetServiceStatus(const QByteArray& serviceId) const
{
	QString retVal = "Undefined";
	if (m_serviceStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr serviceStatusDataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, serviceStatusDataPtr)){
			agentinodata::IServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::IServiceStatusInfo*>(serviceStatusDataPtr.GetPtr());
			if (serviceStatusInfoPtr != nullptr){
				agentinodata::IServiceStatusInfo::ServiceStatus status = serviceStatusInfoPtr->GetServiceStatus();
				agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(status);
				retVal = processStateEnum.id;
			}
		}
	}

	return retVal;
}


QString CServiceCompositeInfoComp::GetDependantServiceStatus(const QByteArray& serviceId) const
{
	QString retVal = "AllRunning";

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
											QString serviceStatus = GetServiceStatus(dependantServiceId);
											if (serviceStatus == "Undefined"){
												return "Undefined";
											}
											else if (serviceStatus == "NotRunning"){
												retVal = "NotAllRunning";
											}
											else if (serviceStatus == "Running" && retVal != "NotAllRunning"){
												retVal = "AllRunning";
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


} // namespace agentinodata


