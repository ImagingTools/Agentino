#include <agentgql/CServiceCollectionControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#undef GetObject
#undef StartService

// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/IAgentInfo.h>


namespace agentgql
{


// reimplemented (sdl::imtbase::ImtCollection::CGraphQlHandlerCompBase)

sdl::imtbase::ImtCollection::CVisualStatus CServiceCollectionControllerComp::OnGetObjectVisualStatus(
	const sdl::imtbase::ImtCollection::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::imtbase::ImtCollection::CVisualStatus retVal = BaseClass::OnGetObjectVisualStatus(getObjectVisualStatusRequest, gqlRequest, errorMessage);
	if (!retVal.Version_1_0){
		I_CRITICAL();
		
		return retVal;
	}
	
	sdl::imtbase::ImtCollection::CVisualStatus::V1_0& response = *retVal.Version_1_0;
	
	if (response.objectId){
		imtbase::IObjectCollection::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
		for (const imtbase::IObjectCollection::Id& elementId : elementIds){
			imtbase::IObjectCollection::DataPtr agentDataPtr;
			if (m_objectCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
				agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
				if (agentInfoPtr != nullptr){
					QString agentName = m_objectCollectionCompPtr->GetElementInfo(elementId, imtbase::ICollectionInfo::EIT_NAME).toString();
					
					imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
					if (serviceCollectionPtr != nullptr){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(*response.objectId, serviceDataPtr)){
							const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr != nullptr){
								QString serviceName = serviceInfoPtr->GetServiceName();
								QString description = serviceInfoPtr->GetServiceDescription();
								
								response.text = serviceName + "@" + agentName;
								response.description = description;
								
								break;
							}
						}
					}
				}
			}
		}
	}
	
	return retVal;
}


// reimplemented (sdl::agentino::Services::CServiceCollectionControllerCompBase)

bool CServiceCollectionControllerComp::CreateRepresentationFromObject(
	const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
	const sdl::agentino::Services::CServicesListGqlRequest& servicesListRequest,
	sdl::agentino::Services::CServiceItem::V1_0& representationObject,
	QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CDeviceCollectionControllerComp");
		return false;
	}
	
	QByteArray objectId = objectCollectionIterator.GetObjectId();
	
	agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (objectCollectionIterator.GetObjectData(serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
	}
	
	if (serviceInfoPtr == nullptr){
		errorMessage = QString("Unable to create representation for service '%1'. Error: Service is invalid");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return false;
	}
	
	sdl::agentino::Services::ServicesListRequestInfo requestInfo = servicesListRequest.GetRequestInfo();
	if (requestInfo.items.isIdRequested){
		representationObject.id = objectId;
	}
	
	if (requestInfo.items.isTypeIdRequested){
		QByteArray typeId = m_objectCollectionCompPtr->GetObjectTypeId(objectId);
		representationObject.typeId = typeId;
	}
	
	if (requestInfo.items.isNameRequested){
		QString name = serviceInfoPtr->GetServiceName();
		representationObject.name = name;
	}
	
	if (requestInfo.items.isDescriptionRequested){
		QString description = serviceInfoPtr->GetServiceDescription();
		representationObject.description = description;
	}
	
	if (requestInfo.items.isStatusRequested){
		if (m_serviceControllerCompPtr.IsValid()){
			agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(objectId);
			agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
			
			representationObject.status = processStateEnum.id;
		}
	}
	
	if (requestInfo.items.isVersionRequested){
		representationObject.version = serviceInfoPtr->GetServiceVersion();
	}
	
	return true;
}


bool CServiceCollectionControllerComp::CreateRepresentationFromObject(
	const istd::IChangeable& data,
	const sdl::agentino::Services::CGetServiceGqlRequest& getServiceRequest,
	sdl::agentino::Services::CServiceData::V1_0& serviceData,
	QString& errorMessage) const
{
	sdl::agentino::Services::GetServiceRequestArguments arguments = getServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT(false);
		return false;
	}
	
	if (!arguments.input.Version_1_0->id.has_value()){
		Q_ASSERT(false);
		return false;
	}
	
	const agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CServiceInfo*>(&data);
	if (serviceInfoPtr == nullptr){
		errorMessage = QString("Unable to create representation for the agent object. Error: Object is invalid");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");
		return false; 
	}
	
	if (!agentinodata::GetRepresentationFromService(serviceData, *serviceInfoPtr)){
		errorMessage = QString("Unable to create representation for the agent object. Error: Representation failed");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");
		return false; 
	}
	
	QByteArray objectId = *arguments.input.Version_1_0->id;
	
	if (m_serviceControllerCompPtr.IsValid()){
		agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(objectId);
		agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
		serviceData.status = processStateEnum.id;
	}
	
	serviceData.id = objectId;
	
	return true;
}


istd::IChangeableUniquePtr CServiceCollectionControllerComp::CreateObjectFromRepresentation(
	const sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation,
	QByteArray& newObjectId,
	QString& errorMessage) const
{
	istd::TUniqueInterfacePtr<agentinodata::IServiceInfo> serviceInstancePtr = m_serviceInfoFactCompPtr.CreateInstance();
	if (!serviceInstancePtr.IsValid()){
		errorMessage = QString("Unable to create service instance. Object is invalid");

		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		
		return nullptr;
	}

	agentinodata::CIdentifiableServiceInfo* serviceInfoImplPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceInstancePtr.GetPtr());
	if (serviceInfoImplPtr == nullptr) {
		errorMessage = QString("Unable to create service instance. Object is invalid");

		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}
	
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoImplPtr, serviceDataRepresentation, errorMessage)){
		errorMessage = QString("Unable to create service from representation. Error: Representation invalid");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return nullptr;
	}
	
	if (serviceDataRepresentation.id){
		newObjectId = *serviceDataRepresentation.id;
		
		serviceInfoImplPtr->SetObjectUuid(newObjectId);
	}
	
	QByteArray serviceName = serviceInfoImplPtr->GetServiceName().toUtf8();
	QByteArray servicePath = serviceInfoImplPtr->GetServicePath();
	
	QFileInfo fileInfo(servicePath);
	if (!fileInfo.exists()){
		errorMessage = QString("Unable to create service from representation. Error: Service path '%1' not exists").arg(qPrintable(servicePath));
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return nullptr;
	}
	
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollectionPtr = GetConnectionCollection(serviceName, servicePath);
	if (connectionCollectionPtr != nullptr){
		serviceInfoImplPtr->SetTracingLevel(connectionCollectionPtr->GetTracingLevel());
		serviceInfoImplPtr->SetServiceVersion(connectionCollectionPtr->GetServiceVersion());
		serviceInfoImplPtr->SetServiceTypeName(connectionCollectionPtr->GetServiceTypeName().toUtf8());
		
		imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfoImplPtr->GetInputConnections();
		imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfoImplPtr->GetDependantServiceConnections();
		
		const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollectionPtr->GetServerConnectionList());
		const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
		if (objectCollection != nullptr){
			imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
			for (const QByteArray& id: ids){
				const imtservice::IServiceConnectionInfo* connectionParamPtr = connectionCollectionPtr->GetConnectionMetaInfo(id);
				if (connectionParamPtr == nullptr){
					continue;
				}
				
				const imtcom::IServerConnectionInterface* serverConnectionPtr  = connectionCollectionPtr->GetServerConnection(id);
				if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
					istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
					urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam);
					
					QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
					QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
					
					urlConnectionParamPtr->SetServiceTypeName(connectionParamPtr->GetServiceTypeName());
					urlConnectionParamPtr->SetUsageId(connectionParamPtr->GetUsageId());
					
					if (url != nullptr){
						urlConnectionParamPtr->SetUrl(*url);
					}
					
					urlConnectionParamPtr->SetConnectionType(connectionParamPtr->GetConnectionType());
					
					incomingConnectionCollectionPtr->InsertNewObject("ConnectionInfo", connectionName, connectionDescription, urlConnectionParamPtr.PopPtr(), id);
				}
				else{
					istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
					urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam);
					
					QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
					QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
					
					urlConnectionLinkParamPtr->SetUsageId(connectionParamPtr->GetUsageId());
					urlConnectionLinkParamPtr->SetServiceTypeName(connectionParamPtr->GetServiceTypeName());
					
					if (url != nullptr){
						urlConnectionLinkParamPtr->SetUrl(*url);
					}
					
					dependantConnectionCollectionPtr->InsertNewObject("ConnectionLink", connectionName, connectionDescription, urlConnectionLinkParamPtr.PopPtr(), id);
				}
			}
		}
	}

	istd::IChangeableUniquePtr retVal;
	retVal.MoveCastedPtr<agentinodata::IServiceInfo>(serviceInstancePtr);
	
	return retVal;
}


bool CServiceCollectionControllerComp::UpdateObjectFromRepresentationRequest(
			const ::imtgql::CGqlRequest& /*rawGqlRequest*/,
			const sdl::agentino::Services::CUpdateServiceGqlRequest& updateServiceRequest,
			istd::IChangeable& object,
			QString& errorMessage) const
{
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(0, "Attribute 'ServiceController' was not set", "CServiceCollectionControllerComp");
		return false;
	}
	
	sdl::agentino::Services::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0){
		I_CRITICAL();
		return false;
	}
	
	if (!arguments.input.Version_1_0->id.has_value()){
		I_CRITICAL();
		return false;
	}
	
	if (!arguments.input.Version_1_0->item.has_value()){
		I_CRITICAL();
		return false;
	}
	
	agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(&object);
	if (serviceInfoPtr == nullptr){
		I_CRITICAL();
		return false;
	}
	
	QByteArray objectId = *arguments.input.Version_1_0->id;
	sdl::agentino::Services::CServiceData::V1_0 serviceData = *arguments.input.Version_1_0->item;
	
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to update service from representation. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return false;
	}
	
	QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeName().toUtf8();
	
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollectionPtr = GetConnectionCollection(objectId);
	if (connectionCollectionPtr != nullptr){
		QString serviceVersion = connectionCollectionPtr->GetServiceVersion();
		serviceInfoPtr->SetServiceVersion(serviceVersion);
		
		bool wasRunning = false;
		agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(objectId);
		if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
			wasRunning = true;
			
			bool wasStopped = m_serviceControllerCompPtr->StopService(objectId);
			if (!wasStopped){
				errorMessage = QString("Service '%1' cannot be stopped").arg(qPrintable(serviceTypeName));
				SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
				return false;
			}
		}
		
		if (!UpdateConnectionCollectionFromService(*serviceInfoPtr, *connectionCollectionPtr)){
			errorMessage = QString("Unable to update connection from the service '%1'").arg(qPrintable(serviceTypeName));
			SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
			return false;
		}
		
		if (wasRunning){
			if (!m_serviceControllerCompPtr->StartService(objectId)){
				errorMessage = QString("Service '%1' cannot be started").arg(qPrintable(serviceTypeName));
				SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
				return false;
			}
		}
	}
	
	return true;
}


// reimplemented (imtservice::IConnectionCollectionProvider)

std::shared_ptr<imtservice::IConnectionCollection> CServiceCollectionControllerComp::GetConnectionCollection(const QByteArray& serviceId) const
{
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollection;
	
	const agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(dataPtr.GetPtr());
	}
	
	if (serviceInfoPtr == nullptr){
		SendErrorMessage(0,
						 QString("Unable to get connection collection for service '%1'. Error: Service is invalid").arg(qPrintable(serviceId)),
						 "CServiceCollectionControllerComp");
		return nullptr;
	}
	
	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	QByteArray serviceName = serviceInfoPtr->GetServiceTypeName().toUtf8();
	
	if (servicePath.isEmpty()){
		SendErrorMessage(0,
						 QString("Unable to get connection collection for service '%1'. Error: Service path is empty").arg(qPrintable(serviceId)),
						 "CServiceCollectionControllerComp");
		return nullptr;
	}
	
	return GetConnectionCollection(serviceName, servicePath);
}


void CServiceCollectionControllerComp::OnComponentDestroyed()
{
	m_pluginMap.clear();
	
	BaseClass::OnComponentDestroyed();
}


imtbase::IObjectCollection* CServiceCollectionControllerComp::GetObjectCollection(const QByteArray& /*id*/) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return nullptr;
	}
	
	istd::TDelPtr<istd::IChangeable> collectionPtr = m_objectCollectionCompPtr->CloneMe();
	if (!collectionPtr.IsValid()){
		return nullptr;
	}
	
	return dynamic_cast<imtbase::IObjectCollection*>(collectionPtr.PopPtr());
}


// private methods

bool CServiceCollectionControllerComp::CheckInputPortsUpdated(
	agentinodata::IServiceInfo& serviceInfo,
	const imtservice::IConnectionCollection& connectionCollection) const
{
	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfo.GetInputConnections();
	if (connectionCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = connectionCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (connectionCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					QString usageId = connectionParamPtr->GetUsageId();
					int servicePort = connectionParamPtr->GetUrl().port();
					const QUrl* actualUrlPtr = connectionCollection.GetUrl(usageId.toUtf8());
					
					if (actualUrlPtr != nullptr && actualUrlPtr->port() != servicePort){
						return true;
					}
				}
			}
		}
	}
	
	return false;
}


bool CServiceCollectionControllerComp::UpdateConnectionCollectionFromService(agentinodata::IServiceInfo& serviceInfo, imtservice::IConnectionCollection& connectionCollection) const
{
	imtbase::IObjectCollection* inputCollectionPtr = serviceInfo.GetInputConnections();
	if (inputCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = inputCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (inputCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					QByteArray usageId = connectionParamPtr->GetUsageId();
					QUrl url = connectionParamPtr->GetUrl();
					
					connectionCollection.SetUrl(usageId, url);
				}
			}
		}
	}
	
	imtbase::IObjectCollection* dependantServiceCollectionPtr = serviceInfo.GetDependantServiceConnections();
	if (dependantServiceCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = dependantServiceCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (dependantServiceCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr != nullptr){
					QByteArray usageId = connectionLinkParamPtr->GetUsageId();
					QUrl url = connectionLinkParamPtr->GetUrl();
					
					connectionCollection.SetUrl(usageId, url);
				}
			}
		}
	}
	
	connectionCollection.SetTracingLevel(serviceInfo.GetTracingLevel());
	
	return true;
}


std::shared_ptr<imtservice::IConnectionCollection> CServiceCollectionControllerComp::GetConnectionCollection(
	const QByteArray& serviceTypeId,
	const QString& servicePath) const
{
	QFileInfo fileInfo(servicePath);
	if (!fileInfo.exists()){
		return nullptr;
	}
	
	QString pluginPath = fileInfo.path() + "/Plugins";
	
	istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceTypeId];
	pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), nullptr));
	
	if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")){
		SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(qPrintable(serviceTypeId)), "CServiceCollectionControllerComp");
		m_pluginMap.remove(serviceTypeId);
	}
	
	if (!m_pluginMap.contains(serviceTypeId)){
		SendErrorMessage(0,
						 QString("Unable to get connection collection for service '%1'. Error: Plugin with name '%2' not found")
							 .arg(qPrintable(serviceTypeId), qPrintable(serviceTypeId)),
						 "CServiceCollectionControllerComp");
		return nullptr;
	}
	
	const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* connectionCollectionFactoryPtr = nullptr;
	for (int index = 0; index < m_pluginMap[serviceTypeId]->m_plugins.count(); index++){
		imtservice::IConnectionCollectionPlugin* pluginPtr = m_pluginMap[serviceTypeId]->m_plugins[index].pluginPtr;
		if (pluginPtr != nullptr){
			connectionCollectionFactoryPtr = pluginPtr->GetConnectionCollectionFactory();
			
			break;
		}
	}
	
	Q_ASSERT(connectionCollectionFactoryPtr != nullptr);
	
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollection;
	connectionCollection.reset(connectionCollectionFactoryPtr->CreateInstance());
	
	return connectionCollection;
}


} // namespace agentgql


