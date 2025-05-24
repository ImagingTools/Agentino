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
	
	if (serviceDataRepresentation.serviceTypeName){
		QString serviceTypeName = *serviceDataRepresentation.serviceTypeName;
		serviceInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	if (serviceDataRepresentation.version){
		QString version = *serviceDataRepresentation.version;
		serviceInfo.SetServiceVersion(version);
	}
	
	if (serviceDataRepresentation.inputConnections){
		QList<sdl::agentino::Services::CInputConnection::V1_0> connections = *serviceDataRepresentation.inputConnections;
		
		imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfo.GetInputConnections();
		if (incomingConnectionCollectionPtr != nullptr){
			incomingConnectionCollectionPtr->ResetData();
			
			for (const sdl::agentino::Services::CInputConnection::V1_0& connection : connections){
				QByteArray id;
				if (connection.id){
					id = *connection.id;
				}
				
				QString name;
				if (connection.connectionName){
					name = *connection.connectionName;
				}
				
				QString description;
				if (connection.description){
					description = *connection.description;
				}
				
				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam);
				
				if (!GetUrlConnectionFromRepresentation(*urlConnectionParamPtr.GetPtr(), connection)){
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
		QList<sdl::agentino::Services::COutputConnection::V1_0> connections = *serviceDataRepresentation.outputConnections;
		
		imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfo.GetDependantServiceConnections();
		if (dependantConnectionCollectionPtr != nullptr){
			dependantConnectionCollectionPtr->ResetData();
			
			for (const sdl::agentino::Services::COutputConnection::V1_0& connection : connections){
				QByteArray id;
				if (connection.id){
					id = *connection.id;
				}
				
				QString name;
				if (connection.connectionName){
					name = *connection.connectionName;
				}
				
				QString description;
				if (connection.description){
					description = *connection.description;
				}
				
				istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
				urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam);
				
				if (!GetUrlConnectionLinkFromRepresentation(*urlConnectionLinkParamPtr.GetPtr(), connection)){
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
	const agentinodata::CServiceInfo& serviceInfo,
	const iprm::IParamsSet* paramsPtr)
{
	agentinodata::CServiceInfo* serviceInfoPtr = const_cast<agentinodata::CServiceInfo*>(&serviceInfo);
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
	
	QString serviceTypeName = serviceInfo.GetServiceTypeName();
	serviceDataRepresentation.serviceTypeName = serviceTypeName;
	
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
	
	serviceDataRepresentation.inputConnections = inputConnectionList;
	
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
	
	serviceDataRepresentation.outputConnections = outputConnectionList;

	return true;
}


bool GetUrlConnectionFromRepresentation(
	imtservice::CUrlConnectionParam& connectionInfo,
	const sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_INPUT);
	
	if (connectionRepresentation.usageId){
		QString usageId = *connectionRepresentation.usageId;
		connectionInfo.SetUsageId(usageId.toUtf8());
	}
	
	if (connectionRepresentation.serviceTypeName){
		QString serviceTypeName = *connectionRepresentation.serviceTypeName;
		connectionInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	QUrl url;
	if (connectionRepresentation.url){
		sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation = *connectionRepresentation.url;

		if (!GetUrlParamFromRepresentation(url, urlRepresentation)){
			return false;
		}
	}
	
	connectionInfo.SetUrl(url);
	
	if (connectionRepresentation.externPorts){
		QList<sdl::agentino::Services::CExternPort::V1_0> externPorts = *connectionRepresentation.externPorts;
		
		for (const sdl::agentino::Services::CExternPort::V1_0& externPort : externPorts){
			imtservice::IServiceConnectionParam::IncomingConnectionParam incomingConnection;
			
			if (externPort.id){
				incomingConnection.id = *externPort.id;
			}
			
			if (externPort.name){
				incomingConnection.name = *externPort.name;
			}
			
			if (externPort.description){
				incomingConnection.description = *externPort.description;
			}
			
			QUrl externalUrl;
			if (externPort.url){
				if (!GetUrlParamFromRepresentation(externalUrl, *externPort.url)){
					return false;
				}
			}

			incomingConnection.url = externalUrl;
			 
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
	QByteArray usageId = connectionInfo.GetUsageId();
	connectionRepresentation.usageId = usageId;
	
	QByteArray serviceTypeName = connectionInfo.GetServiceTypeName();
	connectionRepresentation.serviceTypeName = serviceTypeName;

	QUrl url = connectionInfo.GetUrl();
	
	sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
	if (!GetRepresentationFromUrlParam(url, urlRepresentation)){
		return false;
	}

	connectionRepresentation.url = urlRepresentation;

	QList<sdl::agentino::Services::CExternPort::V1_0> portList;
	
	QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionInfo.GetIncomingConnections();
	for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
		sdl::agentino::Services::CExternPort::V1_0 port;
		port.id = incomingConnection.id;
		port.name = incomingConnection.name;
		port.description = incomingConnection.description;
		
		sdl::agentino::Services::CUrlParameter::V1_0 incomingPortUrlRepresentation;
		if (!GetRepresentationFromUrlParam(incomingConnection.url, incomingPortUrlRepresentation)){
			return false;
		}
		
		port.url = incomingPortUrlRepresentation;
		
		portList << port;
	}
	
	connectionRepresentation.externPorts = portList;
	
	return true;
}


bool GetUrlConnectionLinkFromRepresentation(
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_OUTPUT);
	
	if (connectionRepresentation.usageId){
		QString usageId = *connectionRepresentation.usageId;
		connectionInfo.SetUsageId(usageId.toUtf8());
	}
	
	if (connectionRepresentation.serviceTypeName){
		QString serviceTypeName = *connectionRepresentation.serviceTypeName;
		connectionInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	if (connectionRepresentation.dependantConnectionId){
		QString dependantConnectionId = *connectionRepresentation.dependantConnectionId;
		connectionInfo.SetDependantServiceConnectionId(dependantConnectionId.toUtf8());
	}
	
	if (connectionRepresentation.url){
		QUrl url;
		
		if (!GetUrlParamFromRepresentation(url, *connectionRepresentation.url)){
			return false;
		}

		connectionInfo.SetUrl(url);
	}
	
	return true;
}


bool GetRepresentationFromUrlConnectionLink(
	sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation,
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const iprm::IParamsSet* /*paramsPtr*/)
{
	QByteArray dependantServiceConnectionId = connectionInfo.GetDependantServiceConnectionId();
	QString serviceTypeName = connectionInfo.GetServiceTypeName();
	QByteArray usageId = connectionInfo.GetUsageId();
	
	connectionRepresentation.usageId = usageId;
	connectionRepresentation.dependantConnectionId = dependantServiceConnectionId;
	connectionRepresentation.serviceTypeName = serviceTypeName;
	
	QUrl url = connectionInfo.GetUrl();
	sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
	if (!GetRepresentationFromUrlParam(url, urlRepresentation)){
		return false;
	}

	connectionRepresentation.url = urlRepresentation;

	return true;
}


bool GetUrlParamFromRepresentation(QUrl& url, const sdl::agentino::Services::CUrlParameter::V1_0& urlRepresentation)
{
	if (urlRepresentation.scheme){
		QString scheme = *urlRepresentation.scheme;
		if (scheme.isEmpty()){
			scheme = "http";
		}
		
		url.setScheme(scheme);
	}
	
	if (urlRepresentation.host){
		QString host = *urlRepresentation.host;
		if (host.isEmpty()){
			host = "localhost";
		}
		
		url.setHost(host);
	}
	
	if (urlRepresentation.port){
		int port = *urlRepresentation.port;
		if (port < 0){
			port = 80;
		}
		
		url.setPort(port);
	}
	
	return true;
}


bool GetRepresentationFromUrlParam(const QUrl& url, sdl::agentino::Services::CUrlParameter::V1_0& urlRepresentation)
{
	QString scheme = url.scheme();
	if (scheme.isEmpty()){
		scheme = "http";
	}
	
	urlRepresentation.scheme = scheme;
	
	QString host = url.host();
	if (host.isEmpty()){
		host = "localhost";
	}
	
	urlRepresentation.host = host;
	
	int port = url.port();
	if (port < 0){
		port = 80;
	}
	
	urlRepresentation.port = port;
	
	return true;
}


} // namespace agentinodata


