// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/agentinodata.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>


// Qt includes
#include <QtCore/QFileInfo>

// ACF includes
#include <iprm/TParamsPtr.h>

// ImtCore includes
#include <imtcom/IServerConnectionInterface.h>
#include <imtservice/IConnectionCollection.h>
#include <imtservice/IServiceConnectionParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/ServiceEndpointId.h>


namespace agentinodata
{


namespace
{


/** "ServiceName (host:port)" — what the operator sees in the connection pick-list. */
QString FormatEndpointName(const QString& serviceName, const QString& host, int httpPort)
{
	const QString address = QStringLiteral("%1:%2").arg(host).arg(httpPort);
	if (serviceName.isEmpty()){
		return address;
	}

	return QStringLiteral("%1 (%2)").arg(serviceName, address);
}


} // namespace


ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState)
{
	ProcessStateEnum processStateEnum;

	switch (processState){
	case QProcess::Starting:
		processStateEnum.id = "Starting";
		processStateEnum.name = QString("Starting");

		break;

	case QProcess::Running:
		processStateEnum.id = "Running";
		processStateEnum.name = QString("Running");

		break;

	default:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = QString("Not running");

		break;
	}

	return processStateEnum;
}


bool GetServiceFromRepresentation(
			agentinodata::CServiceInfo& serviceInfo,
			const sdl::V1_0::agentino::CServiceData& serviceDataRepresentation,
			QString& errorMessage)
{
	if (serviceDataRepresentation.name){
		QString name = *serviceDataRepresentation.name;
		serviceInfo.SetServiceName(name);
	}

	if (serviceDataRepresentation.description){
		QString description = *serviceDataRepresentation.description;
		serviceInfo.SetServiceDescription(description);
	}

	if (serviceDataRepresentation.path){
		// Do not require the path to exist on this host: the server mirror stores agent-local
		// paths that are only valid on the agent machine. Existence is checked when the agent
		// starts the service, not when building/mirroring the representation.
		serviceInfo.SetServicePath((*serviceDataRepresentation.path).toUtf8());
	}

	if (serviceDataRepresentation.startScript){
		QString path = *serviceDataRepresentation.startScript;
		serviceInfo.SetStartScriptPath(path.toUtf8());
	}

	if (serviceDataRepresentation.stopScript){
		QString path = *serviceDataRepresentation.stopScript;
		serviceInfo.SetStopScriptPath(path.toUtf8());
	}

	if (serviceDataRepresentation.arguments){
		QByteArray arguments = (*serviceDataRepresentation.arguments).toUtf8();
		serviceInfo.SetServiceArguments(arguments.split(' '));
	}

	if (serviceDataRepresentation.isAutoStart){
		bool isAutoStart = *serviceDataRepresentation.isAutoStart;
		serviceInfo.SetIsAutoStart(isAutoStart);
	}

	if (serviceDataRepresentation.tracingLevel){
		int tracingLevel = *serviceDataRepresentation.tracingLevel;
		serviceInfo.SetTracingLevel(tracingLevel);
	}

	if (serviceDataRepresentation.serviceTypeId){
		QString serviceTypeId = *serviceDataRepresentation.serviceTypeId;
		serviceInfo.SetServiceTypeId(serviceTypeId.toUtf8());
	}

	if (serviceDataRepresentation.version){
		QString version = *serviceDataRepresentation.version;
		serviceInfo.SetServiceVersion(version);
	}

	if (serviceDataRepresentation.settingsPath){
		QString settingsPath = *serviceDataRepresentation.settingsPath;
		serviceInfo.SetServiceSettingsPath(settingsPath.toUtf8());
	}

	if (serviceDataRepresentation.inputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections = *serviceDataRepresentation.inputConnections;

		imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfo.GetInputConnections();
		if (incomingConnectionCollectionPtr != nullptr){
			incomingConnectionCollectionPtr->ResetData();

			for (const istd::TNullableValue<sdl::V1_0::agentino::CInputConnection>& connection : *connections){
				QByteArray id;
				if (connection->id){
					id = *connection->id;
				}

				QString name;
				if (connection->connectionName){
					name = *connection->connectionName;
				}

				QString description;
				if (connection->description){
					description = *connection->description;
				}

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam);

				if (!GetUrlConnectionFromRepresentation(*urlConnectionParamPtr.GetPtr(), *connection)){
					return false;
				}

				QByteArray retVal = incomingConnectionCollectionPtr->InsertNewObject("ConnectionInfo", name, description, urlConnectionParamPtr.PopPtr(), id);
				if (retVal.isEmpty()){
					return false;
				}
			}
		}
	}

	if (serviceDataRepresentation.outputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::COutputConnection>> connections = *serviceDataRepresentation.outputConnections;

		imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfo.GetDependantServiceConnections();
		if (dependantConnectionCollectionPtr != nullptr){
			dependantConnectionCollectionPtr->ResetData();

			for (const istd::TNullableValue<sdl::V1_0::agentino::COutputConnection>& connection : *connections){
				QByteArray id;
				if (connection->id){
					id = *connection->id;
				}

				QString name;
				if (connection->connectionName){
					name = *connection->connectionName;
				}

				QString description;
				if (connection->description){
					description = *connection->description;
				}

				istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
				urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam);

				if (!GetUrlConnectionLinkFromRepresentation(*urlConnectionLinkParamPtr.GetPtr(), *connection)){
					return false;
				}

				QByteArray retVal = dependantConnectionCollectionPtr->InsertNewObject("ConnectionLink", name, description, urlConnectionLinkParamPtr.PopPtr(), id);
				if (retVal.isEmpty()){
					return false;
				}
			}
		}
	}

	return true;
}


bool GetRepresentationFromService(
	sdl::V1_0::agentino::CServiceData& serviceDataRepresentation,
	const CServiceInfo& serviceInfo,
	const iprm::IParamsSet* paramsPtr)
{
	CServiceInfo* serviceInfoPtr = const_cast<CServiceInfo*>(&serviceInfo);
	Q_ASSERT(serviceInfoPtr != nullptr);

	QString serviceName = serviceInfo.GetServiceName();
	serviceDataRepresentation.name = serviceName;

	QString description = serviceInfo.GetServiceDescription();
	serviceDataRepresentation.description = description;

	QString servicePath = serviceInfo.GetServicePath();
	serviceDataRepresentation.path = servicePath;

	QString startScriptPath = serviceInfo.GetStartScriptPath();
	serviceDataRepresentation.startScript = startScriptPath;

	QString stopScriptPath = serviceInfo.GetStopScriptPath();
	serviceDataRepresentation.stopScript = stopScriptPath;

	QString arguments = serviceInfo.GetServiceArguments().join(' ');
	serviceDataRepresentation.arguments = arguments;

	bool isAutoStart = serviceInfo.IsAutoStart();
	serviceDataRepresentation.isAutoStart = isAutoStart;

	int tracingLevel = serviceInfo.GetTracingLevel();
	serviceDataRepresentation.tracingLevel = tracingLevel;

	QString serviceTypeId = serviceInfo.GetServiceTypeId();
	serviceDataRepresentation.serviceTypeId = serviceTypeId;

	QString serviceVersion = serviceInfo.GetServiceVersion();
	serviceDataRepresentation.version = serviceVersion;

	QString settingsPath = QString::fromUtf8(serviceInfo.GetServiceSettingsPath());
	serviceDataRepresentation.settingsPath = settingsPath;

	QList<sdl::V1_0::agentino::CInputConnection> inputConnectionList;

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
	if (connectionCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = connectionCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (connectionCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					sdl::V1_0::agentino::CInputConnection representaion;
					if (!GetRepresentationFromUrlConnection(representaion, *connectionParamPtr, paramsPtr)){
						return false;
					}

					QString connectionName = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
					QString connectionDescription = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

					representaion.id = elementId;
					representaion.connectionName = connectionName;
					representaion.description = connectionDescription;

					inputConnectionList << representaion;
				}
			}
		}
	}

	serviceDataRepresentation.inputConnections.Emplace();
	serviceDataRepresentation.inputConnections->FromList(inputConnectionList);

	QList<sdl::V1_0::agentino::COutputConnection> outputConnectionList;

	imtbase::IObjectCollection* dependantServiceCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
	if (dependantServiceCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = dependantServiceCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (dependantServiceCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr != nullptr){
					sdl::V1_0::agentino::COutputConnection representationModel;
					if (!GetRepresentationFromUrlConnectionLink(representationModel, *connectionLinkParamPtr, paramsPtr)){
						return false;
					}

					QString connectionName = dependantServiceCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
					QString connectionDescription = dependantServiceCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

					representationModel.id = elementId;
					representationModel.connectionName = connectionName;
					representationModel.description = connectionDescription;

					outputConnectionList << representationModel;
				}
			}
		}
	}

	serviceDataRepresentation.outputConnections.Emplace();
	serviceDataRepresentation.outputConnections->FromList(outputConnectionList);

	return true;
}


bool GetUrlConnectionFromRepresentation(
	imtservice::CUrlConnectionParam& connectionInfo,
	const sdl::V1_0::agentino::CInputConnection& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_INPUT);

	if (connectionRepresentation.serviceTypeId){
		QString serviceTypeId = *connectionRepresentation.serviceTypeId;
		connectionInfo.SetServiceTypeId(serviceTypeId.toUtf8());
	}

	if (connectionRepresentation.connectionParam){
		sdl::V1_0::imtbase::CServerConnectionParam urlRepresentation = *connectionRepresentation.connectionParam;

		if (!GetServerConnectionParamFromRepresentation(connectionInfo, urlRepresentation)){
			return false;
		}

		// CServiceConnectionInfo serializes both the base maps and m_defaultConnection.
		// FillDefault from the same base so GetDefaultInterface() and persistence stay consistent
		// (mirror path previously left m_defaultConnection empty → brittle AutoPersistence).
		connectionInfo.SetDefaultServiceInterface(connectionInfo);
	}

	if (connectionRepresentation.externConnectionList){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CExternConnectionInfo>> externConnectionList = *connectionRepresentation.externConnectionList;

		for (const istd::TNullableValue<sdl::V1_0::agentino::CExternConnectionInfo>& externPort : *externConnectionList){
			imtservice::IServiceConnectionParam::IncomingConnectionParam incomingConnection;

			if (externPort->id){
				incomingConnection.SetObjectUuid(*externPort->id);
			}

			if (externPort->connectionParam){
				if (externPort->connectionParam->host){
					incomingConnection.SetHost(*externPort->connectionParam->host);
				}

				if (externPort->connectionParam->httpPort){
					incomingConnection.RegisterProtocol(imtcom::IServerConnectionInterface::PT_HTTP);
					incomingConnection.SetPort(imtcom::IServerConnectionInterface::PT_HTTP, *externPort->connectionParam->httpPort);
					incomingConnection.SetPath(imtcom::IServerConnectionInterface::PT_HTTP, *externPort->connectionParam->httpPath);
				}

				if (externPort->connectionParam->wsPort){
					incomingConnection.RegisterProtocol(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
					incomingConnection.SetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET, *externPort->connectionParam->wsPort);
				}

				if (externPort->connectionParam->isSecure && *externPort->connectionParam->isSecure){
					incomingConnection.SetConnectionFlags(imtcom::IServerConnectionInterface::CF_SECURE);
				}
			}

			connectionInfo.AddExternConnection(incomingConnection);
		}
	}

	return true;
}


bool GetRepresentationFromUrlConnection(
	sdl::V1_0::agentino::CInputConnection& connectionRepresentation,
	imtservice::CUrlConnectionParam& connectionInfo,
	const iprm::IParamsSet* /*paramsPtr*/)
{
	QByteArray serviceTypeId = connectionInfo.GetServiceTypeId();
	connectionRepresentation.serviceTypeId = serviceTypeId;

	sdl::V1_0::imtbase::CServerConnectionParam connectionParamRepresentation;
	if (!GetRepresentationFromServerConnectionParam(connectionInfo, connectionParamRepresentation)){
		return false;
	}

	connectionRepresentation.connectionParam = connectionParamRepresentation;

	QList<sdl::V1_0::agentino::CExternConnectionInfo> externConnectionList;

	imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = connectionInfo.GetIncomingConnections();
	for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
		sdl::V1_0::agentino::CExternConnectionInfo externConnectionInfo;
		externConnectionInfo.id = incomingConnection.GetObjectUuid();

		sdl::V1_0::imtbase::CServerConnectionParam connectionParam;
		connectionParam.host = incomingConnection.GetHost();
		connectionParam.wsPort = incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
		connectionParam.httpPort = incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
		connectionParam.httpPath = incomingConnection.GetPath(imtcom::IServerConnectionInterface::PT_HTTP);
		connectionParam.isSecure = incomingConnection.GetConnectionFlags() == imtcom::IServerConnectionInterface::CF_SECURE;

		externConnectionInfo.connectionParam = connectionParam;

		externConnectionList << externConnectionInfo;
	}
	connectionRepresentation.externConnectionList.Emplace();
	connectionRepresentation.externConnectionList->FromList(externConnectionList);

	return true;
}


bool GetUrlConnectionLinkFromRepresentation(
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const sdl::V1_0::agentino::COutputConnection& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_OUTPUT);

	if (connectionRepresentation.serviceTypeId){
		QString serviceTypeId = *connectionRepresentation.serviceTypeId;
		connectionInfo.SetServiceTypeId(serviceTypeId.toUtf8());
	}

	if (connectionRepresentation.dependantConnectionId){
		QString dependantConnectionId = *connectionRepresentation.dependantConnectionId;
		connectionInfo.SetDependantServiceConnectionId(dependantConnectionId.toUtf8());
	}

	if (connectionRepresentation.connectionParam){
		if (!GetServerConnectionParamFromRepresentation(connectionInfo, *connectionRepresentation.connectionParam)){
			return false;
		}

		// Same as input connections: keep m_defaultConnection aligned with base maps.
		connectionInfo.SetDefaultServiceInterface(connectionInfo);
	}

	return true;
}


bool GetRepresentationFromUrlConnectionLink(
	sdl::V1_0::agentino::COutputConnection& connectionRepresentation,
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const iprm::IParamsSet* /*paramsPtr*/)
{
	QByteArray dependantServiceConnectionId = connectionInfo.GetDependantServiceConnectionId();
	QString serviceTypeId = connectionInfo.GetServiceTypeId();

	connectionRepresentation.dependantConnectionId = dependantServiceConnectionId;
	connectionRepresentation.serviceTypeId = serviceTypeId;

	sdl::V1_0::imtbase::CServerConnectionParam connectionParamRepresentation;
	if (!GetRepresentationFromServerConnectionParam(connectionInfo, connectionParamRepresentation)){
		return false;
	}

	connectionRepresentation.connectionParam = connectionParamRepresentation;

	return true;
}


bool GetServerConnectionParamFromRepresentation(
			imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			const sdl::V1_0::imtbase::CServerConnectionParam& sdlRepresentation)
{
	// Always register both service protocols so SetPort/SetPath never hit the
	// unregistered-protocol path and AutoPersistence never sees a half-built map.
	serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_HTTP);
	serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_WEBSOCKET);

	if (sdlRepresentation.host){
		QString host = *sdlRepresentation.host;
		serverConnectionParam.SetHost(host);
	}

	if (sdlRepresentation.httpPort){
		serverConnectionParam.SetPort(
					imtcom::IServerConnectionInterface::PT_HTTP,
					*sdlRepresentation.httpPort);
	}

	if (sdlRepresentation.wsPort){
		serverConnectionParam.SetPort(
					imtcom::IServerConnectionInterface::PT_WEBSOCKET,
					*sdlRepresentation.wsPort);
	}

	if (sdlRepresentation.httpPath){
		serverConnectionParam.SetPath(
					imtcom::IServerConnectionInterface::PT_HTTP,
					*sdlRepresentation.httpPath);
	}

	if (sdlRepresentation.isSecure){
		bool isSecure = *sdlRepresentation.isSecure;
		if (isSecure){
			serverConnectionParam.SetConnectionFlags(imtcom::IServerConnectionInterface::CF_SECURE);
		}
	}

	return true;
}


bool GetRepresentationFromServerConnectionParam(
			const imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			sdl::V1_0::imtbase::CServerConnectionParam& sdlRepresentation)
{
	const QString host = serverConnectionParam.GetHost();
	sdlRepresentation.host = host;

	int flags = serverConnectionParam.GetConnectionFlags();

	switch (flags){
	case imtcom::IServerConnectionInterface::CF_DEFAULT:
		sdlRepresentation.isSecure = false;
		break;
	case imtcom::IServerConnectionInterface::CF_SECURE:
		sdlRepresentation.isSecure = true;
		break;
	};

	int httpPort = serverConnectionParam.GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
	sdlRepresentation.httpPort = httpPort;

	QString httpPath = serverConnectionParam.GetPath(imtcom::IServerConnectionInterface::PT_HTTP);
	sdlRepresentation.httpPath = httpPath;

	int wsPort = serverConnectionParam.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
	sdlRepresentation.wsPort = wsPort;

	return true;
}



bool GetRepresentationFromConnectionCollection(
			imtservice::IConnectionCollection& connectionCollection,
			sdl::V1_0::agentino::CPluginInfo& connectionCollectionRepresentation)
{
	const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollection.GetServerConnectionList());
	const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
	if (objectCollection != nullptr){
		QList<sdl::V1_0::agentino::CInputConnection> inputConnectionList;
		QList<sdl::V1_0::agentino::COutputConnection> outputConnectionList;

		imtbase::ICollectionInfo::Ids elementIds = collectionInfo->GetElementIds();
		for (const QByteArray& elementId: elementIds){
			const imtservice::IServiceConnectionInfo* connectionParamPtr = connectionCollection.GetConnectionMetaInfo(elementId);
			if (connectionParamPtr == nullptr){
				continue;
			}

			QString connectionName = objectCollection->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
			QString connectionDescription = objectCollection->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

			QByteArray serviceTypeId = connectionParamPtr->GetServiceTypeId();

			sdl::V1_0::imtbase::CServerConnectionParam connectionParamRepresentation;
			// The element's own *current* interface (kept live by SetServerConnectionInterface,
			// e.g. via UpdateConnectionUrl/SetOutputConnection), not GetDefaultInterface() - that
			// is a frozen snapshot taken once when the plugin was first loaded and never updated
			// afterwards, so reading it here would keep showing stale values after any live change.
			// GetRepresentationFromServerConnectionParam() takes the concrete
			// CServerConnectionInterfaceParam, but the collection's own elements are
			// CUrlConnectionParam (a different concrete class) - copy through the interface here.
			const imtcom::IServerConnectionInterface* currentConnectionParamPtr = connectionCollection.GetServerConnection(elementId);
			if (currentConnectionParamPtr != nullptr){
				connectionParamRepresentation.host = currentConnectionParamPtr->GetHost();
				connectionParamRepresentation.isSecure = currentConnectionParamPtr->GetConnectionFlags() == imtcom::IServerConnectionInterface::CF_SECURE;
				connectionParamRepresentation.httpPort = currentConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
				connectionParamRepresentation.httpPath = currentConnectionParamPtr->GetPath(imtcom::IServerConnectionInterface::PT_HTTP);
				connectionParamRepresentation.wsPort = currentConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
			}
			else{
				const imtcom::CServerConnectionInterfaceParam& defaultConnectionParam =
							dynamic_cast<const imtcom::CServerConnectionInterfaceParam&>(connectionParamPtr->GetDefaultInterface());
				if (!GetRepresentationFromServerConnectionParam(defaultConnectionParam, connectionParamRepresentation)){
					return false;
				}
			}

			if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
				sdl::V1_0::agentino::CInputConnection inputConnection;
				inputConnection.id = elementId;
				inputConnection.serviceTypeId = serviceTypeId;
				inputConnection.connectionName = connectionName;
				inputConnection.description = connectionDescription;
				inputConnection.connectionParam = connectionParamRepresentation;
				inputConnectionList << inputConnection;
			}
			else{
				sdl::V1_0::agentino::COutputConnection outputConnection;
				outputConnection.id = elementId;
				outputConnection.serviceTypeId = serviceTypeId;
				outputConnection.connectionName = connectionName;
				outputConnection.description = connectionDescription;
				outputConnection.connectionParam = connectionParamRepresentation;
				outputConnectionList << outputConnection;
			}
		}

		connectionCollectionRepresentation.inputConnections.Emplace();
		connectionCollectionRepresentation.inputConnections->FromList(inputConnectionList);
		connectionCollectionRepresentation.outputConnections.Emplace();
		connectionCollectionRepresentation.outputConnections->FromList(outputConnectionList);

		return true;
	}

	return false;
}


void AppendAvailableConnectionsFromServiceCollection(
			imtbase::IObjectCollection& serviceCollection,
			const QByteArray& connectionUsageId,
			QList<sdl::V1_0::agentino::CDependantConnectionInfo>& outList)
{
	if (connectionUsageId.isEmpty()){
		return;
	}

	const imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollection.GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!serviceCollection.GetObjectData(serviceElementId, serviceDataPtr)){
			continue;
		}

		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
		if (connectionCollectionPtr == nullptr){
			continue;
		}

		const imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (!connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
				continue;
			}

			imtservice::CUrlConnectionParam* connectionParamPtr =
						dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
			if (connectionParamPtr == nullptr){
				continue;
			}

			if (connectionParamPtr->GetConnectionType() != imtservice::IServiceConnectionInfo::CT_INPUT){
				continue;
			}

			if (connectionElementId != connectionUsageId){
				continue;
			}

			sdl::V1_0::imtbase::CServerConnectionParam sdlRepresentation;
			if (!GetRepresentationFromServerConnectionParam(*connectionParamPtr, sdlRepresentation)){
				continue;
			}

			// One candidate per producer endpoint — the stable address to reference.
			// The connection id on its own is only the usage/type marker shared by every
			// service speaking this protocol, so it cannot tell two producers apart:
			// address the endpoint by "<serviceId>|<connectionId>" (unambiguous across
			// agents) and name it after the producing service.
			const QString serviceName = serviceInfoPtr->GetServiceName();

			QString host = connectionParamPtr->GetHost();
			if (host.isEmpty()){
				host = QStringLiteral("localhost");
			}
			const int httpPort = connectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);

			sdl::V1_0::agentino::CDependantConnectionInfo dependantConnectionInfo;
			dependantConnectionInfo.id = ServiceEndpointId::Make(serviceElementId, connectionElementId);
			dependantConnectionInfo.name = FormatEndpointName(serviceName, host, httpPort);
			dependantConnectionInfo.connectionParam = sdlRepresentation;
			outList << dependantConnectionInfo;

			// A producer's extern connections are additional, explicitly configured
			// addresses for this same connection (e.g. exposed through NAT/port-forwarding
			// for access from outside the producer's network). Offer each as its own
			// pickable candidate too, addressed by the incoming connection's own uuid so a
			// consumer can select a specific one instead of always getting the main address.
			const imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections =
						connectionParamPtr->GetIncomingConnections();
			for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incoming : incomingConnections){
				sdl::V1_0::imtbase::CServerConnectionParam externRepresentation;
				if (!GetRepresentationFromServerConnectionParam(incoming, externRepresentation)){
					continue;
				}

				QString externHost = incoming.GetHost();
				if (externHost.isEmpty()){
					externHost = QStringLiteral("localhost");
				}
				const int externHttpPort = incoming.GetPort(imtcom::IServerConnectionInterface::PT_HTTP);

				sdl::V1_0::agentino::CDependantConnectionInfo externConnectionInfo;
				externConnectionInfo.id = ServiceEndpointId::Make(serviceElementId, incoming.GetObjectUuid());
				externConnectionInfo.name = FormatEndpointName(
							serviceName + QStringLiteral(" [extern]"), externHost, externHttpPort);
				externConnectionInfo.connectionParam = externRepresentation;
				outList << externConnectionInfo;
			}
		}
	}
}




} // namespace agentinodata


