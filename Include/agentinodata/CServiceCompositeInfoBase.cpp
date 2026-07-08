// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceCompositeInfoBase.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


// protected methods

QByteArray CServiceCompositeInfoBase::FindServiceIdByUrl(imtbase::IObjectCollection& serviceCollection, const QUrl& url)
{
	imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollection.GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (serviceCollection.GetObjectData(serviceElementId, serviceDataPtr)){
			IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
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
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
					imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
					if (connectionParamPtr != nullptr){
						if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
							QUrl connectionUrl;
							if (connectionParamPtr->GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, connectionUrl)){
								if (connectionUrl == url){
									return serviceElementId;
								}
							}

							if (connectionParamPtr->GetUrl(imtcom::IServerConnectionInterface::PT_WEBSOCKET, connectionUrl)){
								if (connectionUrl == url){
									return serviceElementId;
								}
							}

							imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = connectionParamPtr->GetIncomingConnections();
							for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
								QUrl incomingConnectionUrl;
								incomingConnectionUrl.setHost(incomingConnection.GetHost());
								incomingConnectionUrl.setPort(incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_HTTP));

								if (incomingConnectionUrl.host() == url.host() && incomingConnectionUrl.port() == url.port()){
									return serviceElementId;
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


QByteArray CServiceCompositeInfoBase::FindServiceIdByDependantConnectionId(
			imtbase::IObjectCollection& serviceCollection,
			const QByteArray& dependantServiceConnectionId)
{
	imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollection.GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (serviceCollection.GetObjectData(serviceElementId, serviceDataPtr)){
			IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
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
				if (connectionElementId == dependantServiceConnectionId){
					return serviceElementId;
				}

				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
					imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
					if (connectionParamPtr != nullptr){
						imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = connectionParamPtr->GetIncomingConnections();
						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							if (incomingConnection.GetObjectUuid() == dependantServiceConnectionId){
								return serviceElementId;
							}
						}
					}
				}
			}
		}
	}

	return QByteArray();
}


IServiceCompositeInfo::StateOfRequiredServices CServiceCompositeInfoBase::CalculateStateOfRequiredServices(IServiceInfo& serviceInfo) const
{
	StateOfRequiredServices retVal = SORS_RUNNING;

	// Get Connections
	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfo.GetDependantServiceConnections();
	if (connectionCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr != nullptr){
					QByteArray dependantServiceId = GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
					IServiceStatusInfo::ServiceStatus dependantServiceStatus = GetServiceStatus(dependantServiceId);
					if (dependantServiceStatus == IServiceStatusInfo::SS_UNDEFINED){
						return SORS_UNDEFINED;
					}

					if (dependantServiceStatus == IServiceStatusInfo::SS_NOT_RUNNING){
						retVal = SORS_NOT_RUNNING;
					}
					else if (dependantServiceStatus == IServiceStatusInfo::SS_RUNNING && retVal != SORS_NOT_RUNNING){
						retVal = SORS_RUNNING;
					}
				}
			}
		}
	}

	return retVal;
}


void CServiceCompositeInfoBase::CollectDependencyServices(
			imtbase::IObjectCollection& serviceCollection,
			const QByteArray& serviceId,
			Ids& result) const
{
	imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollection.GetElementIds();
	for (const QByteArray& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (serviceCollection.GetObjectData(serviceElementId, serviceDataPtr)){
			IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
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
							QByteArray dependantServiceId = GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
							if (dependantServiceId == serviceId){
								result << serviceElementId;
							}
						}
					}
				}
			}
		}
	}
}


} // namespace agentinodata
