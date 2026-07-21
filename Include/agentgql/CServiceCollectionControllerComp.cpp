// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#undef GetObject
#undef StartService

// ACF includes
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


namespace agentgql
{


// reimplemented (sdl::V1_0::imtbase::CImtCollectionGqlHandlerCompBase)

sdl::V1_0::imtbase::CVisualStatus CServiceCollectionControllerComp::OnGetObjectVisualStatus(
	const sdl::V1_0::imtbase::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::V1_0::imtbase::CVisualStatus retVal = BaseClass::OnGetObjectVisualStatus(getObjectVisualStatusRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return sdl::V1_0::imtbase::CVisualStatus();
	}
	
	sdl::V1_0::imtbase::CVisualStatus& response = retVal;

	// This controller is bound to the agent's own service collection, so the requested
	// object id addresses a service directly — label it with its name/description.
	if (response.objectId && m_objectCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(*response.objectId, serviceDataPtr)){
			const agentinodata::IServiceInfo* serviceInfoPtr =
						dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr != nullptr){
				response.text = serviceInfoPtr->GetServiceName();
				response.description = serviceInfoPtr->GetServiceDescription();
			}
		}
	}

	return retVal;
}


// reimplemented (sdl::V1_0::agentino::CServiceCollectionControllerCompBase)

bool CServiceCollectionControllerComp::CreateRepresentationFromObject(
	const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
	const sdl::V1_0::agentino::CServicesListGqlRequest& servicesListRequest,
	sdl::V1_0::agentino::CServiceItem& representationObject,
	QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CServiceCollectionControllerComp");
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
	
	sdl::V1_0::agentino::ServicesListRequestInfo requestInfo = servicesListRequest.GetRequestInfo();
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

	if (requestInfo.items.isPathRequested){
		// Required for server mirror seed (Topology / Agents.services) after ServicesList.
		representationObject.path = QString::fromUtf8(serviceInfoPtr->GetServicePath());
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
	const sdl::V1_0::agentino::CGetServiceGqlRequest& getServiceRequest,
	sdl::V1_0::agentino::CServiceData& serviceData,
	QString& errorMessage) const
{
	sdl::V1_0::agentino::GetServiceRequestArguments arguments = getServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value()){
		errorMessage = QStringLiteral("GetService: missing input id");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return false;
	}
	
	const agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CServiceInfo*>(&data);
	if (serviceInfoPtr == nullptr){
		errorMessage = QString("Unable to create representation for the agent object. Error: Object is invalid");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return false; 
	}
	
	if (!agentinodata::GetRepresentationFromService(serviceData, *serviceInfoPtr)){
		errorMessage = QString("Unable to create representation for the agent object. Error: Representation failed");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
		return false; 
	}
	
	QByteArray objectId = *arguments.input->id;
	
	if (m_serviceControllerCompPtr.IsValid()){
		agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(objectId);
		agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
		serviceData.status = processStateEnum.id;
	}
	
	serviceData.id = objectId;

	// The candidate pick-list is no longer stuffed into every GetService — the editor
	// fetches it on demand via the AvailableConnections query. GetService stays cheap.

	return true;
}


istd::IChangeableUniquePtr CServiceCollectionControllerComp::CreateObjectFromRepresentation(
	const sdl::V1_0::agentino::CServiceData& serviceDataRepresentation,
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
	if (serviceInfoImplPtr == nullptr){
		errorMessage = QString("Unable to create service instance. Object is invalid");
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoImplPtr, serviceDataRepresentation, errorMessage)){
		errorMessage = QString("Unable to create service from representation. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}

	if (serviceDataRepresentation.id){
		newObjectId = *serviceDataRepresentation.id;

		serviceInfoImplPtr->SetObjectUuid(newObjectId);
	}

	QByteArray servicePath = serviceInfoImplPtr->GetServicePath();

	QFileInfo fileInfo(servicePath);
	if (!fileInfo.exists()){
		errorMessage = QString("Unable to create service from representation. Error: Service path '%1' not exists").arg(qPrintable(servicePath));
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}

	if (IsServicePathInUse(servicePath, QByteArray())){
		errorMessage = QString("Unable to create service from representation. Error: A service with path '%1' already exists").arg(qPrintable(servicePath));
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}

	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = GetConnectionCollectionByServicePath(servicePath);
	if (connectionCollectionPtr.IsValid()){
		serviceInfoImplPtr->SetTracingLevel(connectionCollectionPtr->GetTracingLevel());
		serviceInfoImplPtr->SetServiceVersion(connectionCollectionPtr->GetServiceVersion());
		serviceInfoImplPtr->SetServiceTypeId(connectionCollectionPtr->GetServiceTypeId().toUtf8());

		PopulateConnectionsFromPlugin(*serviceInfoImplPtr, *connectionCollectionPtr);
	}

	istd::IChangeableUniquePtr retVal;
	retVal.MoveCastedPtr<agentinodata::IServiceInfo>(std::move(serviceInstancePtr));

	return retVal;
}


bool CServiceCollectionControllerComp::UpdateObjectFromRepresentationRequest(
			const ::imtgql::CGqlRequest& /*rawGqlRequest*/,
			const sdl::V1_0::agentino::CUpdateServiceGqlRequest& updateServiceRequest,
			istd::IChangeable& object,
			QString& errorMessage) const
{
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(0, "Attribute 'ServiceController' was not set", "CServiceCollectionControllerComp");

		return false;
	}
	
	sdl::V1_0::agentino::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input){
		I_CRITICAL();

		return false;
	}
	
	if (!arguments.input->id.has_value()){
		I_CRITICAL();

		return false;
	}
	
	if (!arguments.input->item.has_value()){
		I_CRITICAL();

		return false;
	}
	
	agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(&object);
	if (serviceInfoPtr == nullptr){
		I_CRITICAL();

		return false;
	}
	
	QByteArray objectId = *arguments.input->id;
	sdl::V1_0::agentino::CServiceData serviceData = *arguments.input->item;
	
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to update service from representation. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return false;
	}

	if (IsServicePathInUse(serviceInfoPtr->GetServicePath(), objectId)){
		errorMessage = QString("Unable to update service from representation. Error: A service with path '%1' already exists").arg(qPrintable(serviceInfoPtr->GetServicePath()));
		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return false;
	}

	QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeId().toUtf8();

	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = GetConnectionCollectionByServiceId(objectId);
	if (connectionCollectionPtr.IsValid()){
		serviceInfoPtr->SetServiceVersion(connectionCollectionPtr->GetServiceVersion());

		// Only touch the running instance when the wiring it uses actually changed.
		// Renaming a service or toggling autostart must not interrupt it.
		if (!IsConnectionUpdateRequired(*serviceInfoPtr, *connectionCollectionPtr)){
			return true;
		}

		bool wasRunning = false;
		agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(objectId);
		if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
			wasRunning = true;

			if (!m_serviceControllerCompPtr->StopService(objectId)){
				errorMessage = QString("Service '%1' cannot be stopped").arg(qPrintable(serviceTypeName));
				SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

				return false;
			}
		}

		const bool connectionsApplied = UpdateConnectionCollectionFromService(*serviceInfoPtr, *connectionCollectionPtr);
		if (!connectionsApplied){
			errorMessage = QString("Unable to update connection from the service '%1'").arg(qPrintable(serviceTypeName));
			SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");
			// Fall through: we stopped the service, so we must still try to bring it back.
		}

		if (wasRunning){
			if (!m_serviceControllerCompPtr->StartService(objectId)){
				errorMessage = QString("Service '%1' cannot be started").arg(qPrintable(serviceTypeName));
				SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

				return false;
			}
		}

		return connectionsApplied;
	}

	return true;
}


// reimplemented (imtservice::IConnectionCollectionProvider)

imtservice::IConnectionCollectionSharedPtr CServiceCollectionControllerComp::GetConnectionCollectionByServicePath(const QString& servicePath) const
{
	const QFileInfo fi(servicePath);
	if (!fi.exists()){
		return nullptr;
	}

	const QString pluginPath = fi.dir().absoluteFilePath(QStringLiteral("Plugins"));
	const QByteArray key = servicePath.toUtf8();

	if (m_pluginMap.contains(key)){
		return m_pluginMap[key].connectionCollectionPtr;
	}

	// Build the entry locally and cache it only on success: a service whose plugin
	// directory appears later (files still being deployed) must be retried on the
	// next call, not poisoned by a cached failure.
	PluginInfo pluginInfo;
	pluginInfo.pluginManagerPtr.SetPtr(new PluginManager(
											IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings),
											IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings),
											nullptr));

	if (!pluginInfo.pluginManagerPtr.IsValid() || !pluginInfo.pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")){
		SendErrorMessage(0,
						 QStringLiteral("Unable to load a plugin for '%1'").arg(servicePath),
						 QStringLiteral("CServiceCollectionControllerComp"));
		return nullptr;
	}

	const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* connectionCollectionFactoryPtr = nullptr;
	for (int i = 0; i < pluginInfo.pluginManagerPtr->m_plugins.count(); ++i){
		auto* plugin = pluginInfo.pluginManagerPtr->m_plugins[i].pluginPtr;
		if (!plugin)
			continue;

		if (plugin->GetPluginName() != fi.baseName() + "Settings"){
			continue;
		}

		connectionCollectionFactoryPtr = plugin->GetConnectionCollectionFactory();
		if (connectionCollectionFactoryPtr != nullptr){
			break;
		}
	}

	if (connectionCollectionFactoryPtr == nullptr){
		SendErrorMessage(0,
						 QStringLiteral("Plugin for '%1' does not provide a connection collection factory")
						 .arg(servicePath),
						 QStringLiteral("CServiceCollectionControllerComp"));
		return nullptr;
	}

	istd::TUniqueInterfacePtr<imtservice::IConnectionCollection> connectionCollectionPtr = connectionCollectionFactoryPtr->CreateInstance();
	if (!connectionCollectionPtr.IsValid()){
		SendErrorMessage(0,
						 QStringLiteral("Failed to create connection collection instance for '%1'")
						 .arg(servicePath),
						 QStringLiteral("CServiceCollectionControllerComp"));
		return nullptr;
	}

	pluginInfo.connectionCollectionPtr.FromUnique(std::move(connectionCollectionPtr));

	PluginInfo& cachedInfo = m_pluginMap[key];
	cachedInfo.pluginManagerPtr.SetPtr(pluginInfo.pluginManagerPtr.PopPtr());
	cachedInfo.connectionCollectionPtr = pluginInfo.connectionCollectionPtr;

	return cachedInfo.connectionCollectionPtr;
}


imtservice::IConnectionCollectionSharedPtr CServiceCollectionControllerComp::GetConnectionCollectionByServiceId(const QByteArray& serviceId) const
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
	if (servicePath.isEmpty()){
		SendErrorMessage(0,
						 QString("Unable to get connection collection for service '%1'. Error: Service path is empty").arg(qPrintable(serviceId)),
						 "CServiceCollectionControllerComp");

		return nullptr;
	}

	return GetConnectionCollectionByServicePath(servicePath);
}


void CServiceCollectionControllerComp::OnComponentDestroyed()
{
	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
}


// private methods

bool CServiceCollectionControllerComp::IsServicePathInUse(const QByteArray& servicePath, const QByteArray& excludeObjectId) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return false;
	}

	imtbase::ICollectionInfo::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		if (elementId == excludeObjectId){
			continue;
		}

		const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(m_objectCollectionCompPtr->GetObjectPtr(elementId));
		if (serviceInfoPtr == nullptr){
			continue;
		}

		if (QString(serviceInfoPtr->GetServicePath()).compare(QString(servicePath), Qt::CaseInsensitive) == 0){
			return true;
		}
	}

	return false;
}


namespace
{


/**
	True when \a serviceParam (host + HTTP/WS ports from the edited descriptor) differs
	from what the plugin's connection collection currently holds for the same id.
	An id the plugin does not track cannot be applied to it, so it never forces a restart.
*/
bool IsConnectionInterfaceChanged(
			const imtservice::IConnectionCollection& connectionCollection,
			const QByteArray& connectionId,
			const imtcom::IServerConnectionInterface& serviceParam)
{
	const imtcom::IServerConnectionInterface* currentParamPtr =
				connectionCollection.GetServerConnection(connectionId);
	if (currentParamPtr == nullptr){
		return false;
	}

	if (currentParamPtr->GetHost() != serviceParam.GetHost()){
		return true;
	}

	return currentParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP)
					!= serviceParam.GetPort(imtcom::IServerConnectionInterface::PT_HTTP)
				|| currentParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET)
					!= serviceParam.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
}


/** Any element of \a connections (inputs or dependant links) whose interface changed? */
bool HasChangedConnection(
			imtbase::IObjectCollection* connections,
			const imtservice::IConnectionCollection& connectionCollection)
{
	if (connections == nullptr){
		return false;
	}

	const imtbase::ICollectionInfo::Ids elementIds = connections->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		if (!connections->GetObjectData(elementId, connectionDataPtr)){
			continue;
		}

		// Both CUrlConnectionParam and CUrlConnectionLinkParam are connection interfaces.
		const imtcom::IServerConnectionInterface* serviceParamPtr =
					dynamic_cast<const imtcom::IServerConnectionInterface*>(connectionDataPtr.GetPtr());
		if (serviceParamPtr != nullptr
					&& IsConnectionInterfaceChanged(connectionCollection, elementId, *serviceParamPtr)){
			return true;
		}
	}

	return false;
}


} // namespace


bool CServiceCollectionControllerComp::IsConnectionUpdateRequired(
			agentinodata::IServiceInfo& serviceInfo,
			const imtservice::IConnectionCollection& connectionCollection) const
{
	if (serviceInfo.GetTracingLevel() != connectionCollection.GetTracingLevel()){
		return true;
	}

	return HasChangedConnection(serviceInfo.GetInputConnections(), connectionCollection)
				|| HasChangedConnection(serviceInfo.GetDependantServiceConnections(), connectionCollection);
}


void CServiceCollectionControllerComp::PopulateConnectionsFromPlugin(
			agentinodata::IServiceInfo& serviceInfo,
			const imtservice::IConnectionCollection& connectionCollection) const
{
	imtbase::IObjectCollection* inputConnectionsPtr = serviceInfo.GetInputConnections();
	imtbase::IObjectCollection* dependantConnectionsPtr = serviceInfo.GetDependantServiceConnections();
	if (inputConnectionsPtr == nullptr || dependantConnectionsPtr == nullptr){
		return;
	}

	const imtbase::ICollectionInfo* connectionListPtr =
				dynamic_cast<const imtbase::ICollectionInfo*>(connectionCollection.GetServerConnectionList());
	if (connectionListPtr == nullptr){
		return;
	}

	const imtbase::ICollectionInfo::Ids connectionIds = connectionListPtr->GetElementIds();
	for (const QByteArray& connectionId : connectionIds){
		const imtservice::IServiceConnectionInfo* metaInfoPtr =
					connectionCollection.GetConnectionMetaInfo(connectionId);
		if (metaInfoPtr == nullptr){
			continue;
		}

		const imtservice::IServiceConnectionInfo::ConnectionType connectionType = metaInfoPtr->GetConnectionType();
		const bool isInput = (connectionType == imtservice::IServiceConnectionInfo::CT_INPUT);
		imtbase::IObjectCollection* targetCollectionPtr = isInput ? inputConnectionsPtr : dependantConnectionsPtr;
		if (targetCollectionPtr->GetElementIds().contains(connectionId)){
			continue;
		}

		istd::TDelPtr<imtservice::CServiceConnectionInfo> connectionParamPtr;
		if (isInput){
			connectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam);
		}
		else{
			connectionParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam);
		}

		connectionParamPtr->SetConnectionType(connectionType);
		connectionParamPtr->SetServiceTypeId(metaInfoPtr->GetServiceTypeId());

		const imtcom::IServerConnectionInterface& defaultInterface = metaInfoPtr->GetDefaultInterface();
		connectionParamPtr->SetDefaultServiceInterface(defaultInterface);
		connectionParamPtr->SetHost(defaultInterface.GetHost());
		connectionParamPtr->SetPort(
					imtcom::IServerConnectionInterface::PT_HTTP,
					defaultInterface.GetPort(imtcom::IServerConnectionInterface::PT_HTTP));
		connectionParamPtr->SetPort(
					imtcom::IServerConnectionInterface::PT_WEBSOCKET,
					defaultInterface.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET));

		const QString connectionName =
					connectionListPtr->GetElementInfo(connectionId, imtbase::ICollectionInfo::EIT_NAME).toString();
		const QString connectionDescription =
					connectionListPtr->GetElementInfo(connectionId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();

		targetCollectionPtr->InsertNewObject(
					isInput ? QByteArrayLiteral("ConnectionInfo") : QByteArrayLiteral("ConnectionLink"),
					connectionName,
					connectionDescription,
					connectionParamPtr.PopPtr(),
					connectionId);
	}
}


bool CServiceCollectionControllerComp::UpdateConnectionCollectionFromService(agentinodata::IServiceInfo& serviceInfo, imtservice::IConnectionCollection& connectionCollection) const
{
	bool retVal = true;

	imtbase::IObjectCollection* inputCollectionPtr = serviceInfo.GetInputConnections();
	if (inputCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = inputCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (inputCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					imtcom::CServerConnectionInterfaceParam serverConnectionParam;
					serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_HTTP);
					serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_WEBSOCKET);

					serverConnectionParam.SetHost(connectionParamPtr->GetHost());

					imtcom::IServerConnectionInterface::ProtocolTypes protocols = connectionParamPtr->GetSupportedProtocols();
					for (const imtcom::CServerConnectionInterfaceParam::ProtocolType& protocolType: protocols){
						serverConnectionParam.SetPort(protocolType, connectionParamPtr->GetPort(protocolType));
					}

					if (!connectionCollection.SetServerConnectionInterface(elementId, serverConnectionParam)){
						SendErrorMessage(0,
									QString("Input connection '%1' is not tracked by the service's settings plugin — its URL was not applied")
												.arg(QString::fromUtf8(elementId)),
									"CServiceCollectionControllerComp");
						retVal = false;
					}
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
					imtcom::CServerConnectionInterfaceParam serverConnectionParam;
					serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_HTTP);
					serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_WEBSOCKET);

					serverConnectionParam.SetHost(connectionLinkParamPtr->GetHost());
					imtcom::IServerConnectionInterface::ProtocolTypes protocols = connectionLinkParamPtr->GetSupportedProtocols();

					for (const imtcom::CServerConnectionInterfaceParam::ProtocolType& protocolType: protocols){
						serverConnectionParam.SetPort(protocolType, connectionLinkParamPtr->GetPort(protocolType));
					}

					if (!connectionCollection.SetServerConnectionInterface(elementId, serverConnectionParam)){
						SendErrorMessage(0,
									QString("Output connection '%1' is not tracked by the service's settings plugin — its URL was not applied")
												.arg(QString::fromUtf8(elementId)),
									"CServiceCollectionControllerComp");
						retVal = false;
					}
				}
			}
		}
	}

	connectionCollection.SetTracingLevel(serviceInfo.GetTracingLevel());

	return retVal;
}


} // namespace agentgql


