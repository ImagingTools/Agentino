// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#include <agentinodata/agentinodata.h>


// Qt includes
#include <QtCore/QFileInfo>

// ACF includes
#include <iprm/TParamsPtr.h>

// ImtCore includes
#include <imtservice/IConnectionCollection.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentinodata
{


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
			const sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation,
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
		QString path = *serviceDataRepresentation.path;

		QFileInfo fileInfo(path);
		if (!fileInfo.exists()){
			errorMessage = QString("Service path '%1' not exists").arg(qPrintable(path));

			return false;
		}

		serviceInfo.SetServicePath(path.toUtf8());
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

	if (serviceDataRepresentation.inputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CInputConnection::V1_0>> connections = *serviceDataRepresentation.inputConnections;

		imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfo.GetInputConnections();
		if (incomingConnectionCollectionPtr != nullptr){
			incomingConnectionCollectionPtr->ResetData();

			for (const istd::TSharedNullable<sdl::agentino::Services::CInputConnection::V1_0>& connection : *connections){
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
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::COutputConnection::V1_0>> connections = *serviceDataRepresentation.outputConnections;

		imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfo.GetDependantServiceConnections();
		if (dependantConnectionCollectionPtr != nullptr){
			dependantConnectionCollectionPtr->ResetData();

			for (const istd::TSharedNullable<sdl::agentino::Services::COutputConnection::V1_0>& connection : *connections){
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
	sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation,
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

	QList<sdl::agentino::Services::CInputConnection::V1_0> inputConnectionList;

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
	if (connectionCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = connectionCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (connectionCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					sdl::agentino::Services::CInputConnection::V1_0 representaion;
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

	QList<sdl::agentino::Services::COutputConnection::V1_0> outputConnectionList;

	imtbase::IObjectCollection* dependantServiceCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
	if (dependantServiceCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = dependantServiceCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (dependantServiceCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr != nullptr){
					sdl::agentino::Services::COutputConnection::V1_0 representationModel;
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
	const sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_INPUT);

	if (connectionRepresentation.serviceTypeId){
		QString serviceTypeId = *connectionRepresentation.serviceTypeId;
		connectionInfo.SetServiceTypeId(serviceTypeId.toUtf8());
	}

	if (connectionRepresentation.connectionParam){
		sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 urlRepresentation = *connectionRepresentation.connectionParam;

		if (!GetServerConnectionParamFromRepresentation(connectionInfo, urlRepresentation)){
			return false;
		}
	}

	if (connectionRepresentation.externConnectionList){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CExternConnectionInfo::V1_0>> externConnectionList = *connectionRepresentation.externConnectionList;

		for (const istd::TSharedNullable<sdl::agentino::Services::CExternConnectionInfo::V1_0>& externPort : *externConnectionList){
			imtservice::IServiceConnectionParam::IncomingConnectionParam incomingConnection;

			if (externPort->id){
				incomingConnection.id = *externPort->id;
			}

			if (externPort->description){
				incomingConnection.description = *externPort->description;
			}

			if (externPort->connectionParam){
				if (externPort->connectionParam->host){
					incomingConnection.host = *externPort->connectionParam->host;
				}

				if (externPort->connectionParam->httpPort){
					incomingConnection.httpPort = *externPort->connectionParam->httpPort;
				}

				if (externPort->connectionParam->wsPort){
					incomingConnection.wsPort = *externPort->connectionParam->wsPort;
				}
			}

			connectionInfo.AddExternConnection(incomingConnection);
		}
	}

	return true;
}


bool GetRepresentationFromUrlConnection(
	sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation,
	imtservice::CUrlConnectionParam& connectionInfo,
	const iprm::IParamsSet* /*paramsPtr*/)
{
	QByteArray serviceTypeId = connectionInfo.GetServiceTypeId();
	connectionRepresentation.serviceTypeId = serviceTypeId;

	sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParamRepresentation;
	if (!GetRepresentationFromServerConnectionParam(connectionInfo, connectionParamRepresentation)){
		return false;
	}

	connectionRepresentation.connectionParam = connectionParamRepresentation;

	QList<sdl::agentino::Services::CExternConnectionInfo::V1_0> externConnectionList;

	QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionInfo.GetIncomingConnections();
	for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
		sdl::agentino::Services::CExternConnectionInfo::V1_0 externConnectionInfo;
		externConnectionInfo.description = incomingConnection.description;
		externConnectionInfo.id = incomingConnection.id;

		sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParam;
		connectionParam.host = incomingConnection.host;
		connectionParam.wsPort = incomingConnection.wsPort;
		connectionParam.httpPort = incomingConnection.httpPort;

		externConnectionInfo.connectionParam = connectionParam;

		externConnectionList << externConnectionInfo;
	}
	connectionRepresentation.externConnectionList.Emplace();
	connectionRepresentation.externConnectionList->FromList(externConnectionList);

	return true;
}


bool GetUrlConnectionLinkFromRepresentation(
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation)
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
	}

	return true;
}


bool GetRepresentationFromUrlConnectionLink(
	sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation,
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const iprm::IParamsSet* /*paramsPtr*/)
{
	QByteArray dependantServiceConnectionId = connectionInfo.GetDependantServiceConnectionId();
	QString serviceTypeId = connectionInfo.GetServiceTypeId();

	connectionRepresentation.dependantConnectionId = dependantServiceConnectionId;
	connectionRepresentation.serviceTypeId = serviceTypeId;

	sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParamRepresentation;
	if (!GetRepresentationFromServerConnectionParam(connectionInfo, connectionParamRepresentation)){
		return false;
	}

	connectionRepresentation.connectionParam = connectionParamRepresentation;

	return true;
}


bool GetServerConnectionParamFromRepresentation(
			imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			const sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& sdlRepresentation)
{
	if (sdlRepresentation.host){
		QString host = *sdlRepresentation.host;
		serverConnectionParam.SetHost(host);
	}

	if (sdlRepresentation.httpPort){
		int httpPort = *sdlRepresentation.httpPort;
		serverConnectionParam.SetPort(imtcom::IServerConnectionInterface::PT_HTTP, httpPort);
	}

	if (sdlRepresentation.wsPort){
		int wsPort = *sdlRepresentation.wsPort;
		serverConnectionParam.SetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET, wsPort);
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
			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& sdlRepresentation)
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

	int wsPort = serverConnectionParam.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
	sdlRepresentation.wsPort = wsPort;

	return true;
}



bool GetRepresentationFromConnectionCollection(
			imtservice::IConnectionCollection& connectionCollection,
			sdl::agentino::Services::CPluginInfo::V1_0& connectionCollectionRepresentation)
{
	const imtbase::ICollectionInfo* collectionInfo = static_cast<const imtbase::ICollectionInfo*>(connectionCollection.GetServerConnectionList());
	const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
	if (objectCollection != nullptr){
		QList<sdl::agentino::Services::CInputConnection::V1_0> inputConnectionList;
		QList<sdl::agentino::Services::COutputConnection::V1_0> outputConnectionList;

		imtbase::ICollectionInfo::Ids elementIds = collectionInfo->GetElementIds();
		for (const QByteArray& elementId: elementIds){
			const imtservice::IServiceConnectionInfo* connectionParamPtr = connectionCollection.GetConnectionMetaInfo(elementId);
			if (connectionParamPtr == nullptr){
				continue;
			}

			QString connectionName = objectCollection->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
			QString connectionDescription = objectCollection->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

			QByteArray serviceTypeId = connectionParamPtr->GetServiceTypeId();

			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParamRepresentation;
			const imtcom::CServerConnectionInterfaceParam& defaultConnectionParam = dynamic_cast<const imtcom::CServerConnectionInterfaceParam&>(connectionParamPtr->GetDefaultInterface());
			if (!GetRepresentationFromServerConnectionParam(defaultConnectionParam, connectionParamRepresentation)){
				return false;
			}

			if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
				sdl::agentino::Services::CInputConnection::V1_0 inputConnection;
				inputConnection.id = elementId;
				inputConnection.serviceTypeId = serviceTypeId;
				inputConnection.connectionName = connectionName;
				inputConnection.description = connectionDescription;
				inputConnection.connectionParam = connectionParamRepresentation;
				inputConnectionList << inputConnection;
			}
			else{
				sdl::agentino::Services::COutputConnection::V1_0 outputConnection;
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


} // namespace agentinodata


