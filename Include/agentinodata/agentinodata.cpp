#include <agentinodata/agentinodata.h>


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

	switch (processState) {
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


bool GetServiceFromRepresentation(agentinodata::CServiceInfo& serviceInfo, const sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation)
{
	if (serviceDataRepresentation.Name){
		QString name = *serviceDataRepresentation.Name;
		serviceInfo.SetServiceName(name);
	}
	
	if (serviceDataRepresentation.Description){
		QString description = *serviceDataRepresentation.Description;
		serviceInfo.SetServiceDescription(description);
	}
	
	if (serviceDataRepresentation.Path){
		QString path = *serviceDataRepresentation.Path;
		serviceInfo.SetServicePath(path.toUtf8());
	}
	
	if (serviceDataRepresentation.StartScript){
		QString path = *serviceDataRepresentation.StartScript;
		serviceInfo.SetStartScriptPath(path.toUtf8());
	}
	
	if (serviceDataRepresentation.StopScript){
		QString path = *serviceDataRepresentation.StopScript;
		serviceInfo.SetStopScriptPath(path.toUtf8());
	}
	
	if (serviceDataRepresentation.Arguments){
		QByteArray arguments = (*serviceDataRepresentation.Arguments).toUtf8();
		serviceInfo.SetServiceArguments(arguments.split(' '));
	}
	
	if (serviceDataRepresentation.IsAutoStart){
		bool isAutoStart = *serviceDataRepresentation.IsAutoStart;
		serviceInfo.SetIsAutoStart(isAutoStart);
	}
	
	if (serviceDataRepresentation.TracingLevel){
		int tracingLevel = *serviceDataRepresentation.TracingLevel;
		serviceInfo.SetTracingLevel(tracingLevel);
	}
	
	if (serviceDataRepresentation.ServiceTypeName){
		QString serviceTypeName = *serviceDataRepresentation.ServiceTypeName;
		serviceInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	if (serviceDataRepresentation.Version){
		QString version = *serviceDataRepresentation.Version;
		serviceInfo.SetServiceVersion(version);
	}
	
	if (serviceDataRepresentation.InputConnections){
		QList<sdl::agentino::Services::CInputConnection::V1_0> connections = *serviceDataRepresentation.InputConnections;
		
		imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfo.GetInputConnections();
		if (incomingConnectionCollectionPtr != nullptr){
			incomingConnectionCollectionPtr->ResetData();
			
			for (const sdl::agentino::Services::CInputConnection::V1_0& connection : connections){
				QByteArray id;
				if (connection.Id){
					id = *connection.Id;
				}
				
				QString name;
				if (connection.ConnectionName){
					name = *connection.ConnectionName;
				}
				
				QString description;
				if (connection.Description){
					description = *connection.Description;
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
	
	if (serviceDataRepresentation.OutputConnections){
		QList<sdl::agentino::Services::COutputConnection::V1_0> connections = *serviceDataRepresentation.OutputConnections;
		
		imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfo.GetDependantServiceConnections();
		if (dependantConnectionCollectionPtr != nullptr){
			dependantConnectionCollectionPtr->ResetData();
			
			for (const sdl::agentino::Services::COutputConnection::V1_0& connection : connections){
				QByteArray id;
				if (connection.Id){
					id = *connection.Id;
				}
				
				QString name;
				if (connection.ConnectionName){
					name = *connection.ConnectionName;
				}
				
				QString description;
				if (connection.Description){
					description = *connection.Description;
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
	serviceDataRepresentation.Name = serviceName;
	
	QString description = serviceInfo.GetServiceDescription();
	serviceDataRepresentation.Description = description;
	
	QString servicePath = serviceInfo.GetServicePath();
	serviceDataRepresentation.Path = servicePath;
	
	QString startScriptPath = serviceInfo.GetStartScriptPath();
	serviceDataRepresentation.StartScript = startScriptPath;
	
	QString stopScriptPath = serviceInfo.GetStopScriptPath();
	serviceDataRepresentation.StopScript = stopScriptPath;

	QString arguments = serviceInfo.GetServiceArguments().join(' ');
	serviceDataRepresentation.Arguments = arguments;
	
	bool isAutoStart = serviceInfo.IsAutoStart();
	serviceDataRepresentation.IsAutoStart = isAutoStart;

	int tracingLevel = serviceInfo.GetTracingLevel();
	serviceDataRepresentation.TracingLevel = tracingLevel;
	
	QString serviceTypeName = serviceInfo.GetServiceTypeName();
	serviceDataRepresentation.ServiceTypeName = serviceTypeName;
	
	QString serviceVersion = serviceInfo.GetServiceVersion();
	serviceDataRepresentation.Version = serviceVersion;
	
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
					
					representaion.Id = elementId;
					representaion.ConnectionName = connectionName;
					representaion.Description = connectionDescription;
					
					inputConnectionList << representaion;
				}
			}
		}
	}
	
	serviceDataRepresentation.InputConnections = inputConnectionList;
	
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
					
					representationModel.Id = elementId;
					representationModel.ConnectionName = connectionName;
					representationModel.Description = connectionDescription;
					
					outputConnectionList << representationModel;
				}
			}
		}
	}
	
	serviceDataRepresentation.OutputConnections = outputConnectionList;

	return true;
}


bool GetUrlConnectionFromRepresentation(
	imtservice::CUrlConnectionParam& connectionInfo,
	const sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_INPUT);
	
	if (connectionRepresentation.UsageId){
		QString usageId = *connectionRepresentation.UsageId;
		connectionInfo.SetUsageId(usageId.toUtf8());
	}
	
	if (connectionRepresentation.ServiceTypeName){
		QString serviceTypeName = *connectionRepresentation.ServiceTypeName;
		connectionInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	QUrl url;
	if (connectionRepresentation.Url){
		sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation = *connectionRepresentation.Url;

		if (!GetUrlParamFromRepresentation(url, urlRepresentation)){
			return false;
		}
	}
	
	connectionInfo.SetUrl(url);
	
	if (connectionRepresentation.ExternPorts){
		QList<sdl::agentino::Services::CExternPort::V1_0> externPorts = *connectionRepresentation.ExternPorts;
		
		for (const sdl::agentino::Services::CExternPort::V1_0& externPort : externPorts){
			imtservice::IServiceConnectionParam::IncomingConnectionParam incomingConnection;
			
			if (externPort.Id){
				incomingConnection.id = *externPort.Id;
			}
			
			if (externPort.Name){
				incomingConnection.name = *externPort.Name;
			}
			
			if (externPort.Description){
				incomingConnection.description = *externPort.Description;
			}
			
			QUrl externalUrl;
			if (externPort.Url){
				if (!GetUrlParamFromRepresentation(externalUrl, *externPort.Url)){
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
	connectionRepresentation.UsageId = usageId;
	
	QByteArray serviceTypeName = connectionInfo.GetServiceTypeName();
	connectionRepresentation.ServiceTypeName = serviceTypeName;

	QUrl url = connectionInfo.GetUrl();
	
	sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
	if (!GetRepresentationFromUrlParam(url, urlRepresentation)){
		return false;
	}

	connectionRepresentation.Url = urlRepresentation;

	QList<sdl::agentino::Services::CExternPort::V1_0> portList;
	
	QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionInfo.GetIncomingConnections();
	for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
		sdl::agentino::Services::CExternPort::V1_0 port;
		port.Id = incomingConnection.id;
		port.Name = incomingConnection.name;
		port.Description = incomingConnection.description;
		
		sdl::agentino::Services::CUrlParameter::V1_0 incomingPortUrlRepresentation;
		if (!GetRepresentationFromUrlParam(incomingConnection.url, incomingPortUrlRepresentation)){
			return false;
		}
		
		port.Url = incomingPortUrlRepresentation;
		
		portList << port;
	}
	
	connectionRepresentation.ExternPorts = portList;
	
	return true;
}


bool GetUrlConnectionLinkFromRepresentation(
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation)
{
	connectionInfo.SetConnectionType(imtservice::IServiceConnectionInfo::CT_OUTPUT);
	
	if (connectionRepresentation.UsageId){
		QString usageId = *connectionRepresentation.UsageId;
		connectionInfo.SetUsageId(usageId.toUtf8());
	}
	
	if (connectionRepresentation.ServiceTypeName){
		QString serviceTypeName = *connectionRepresentation.ServiceTypeName;
		connectionInfo.SetServiceTypeName(serviceTypeName.toUtf8());
	}
	
	if (connectionRepresentation.DependantConnectionId){
		QString dependantConnectionId = *connectionRepresentation.DependantConnectionId;
		connectionInfo.SetDependantServiceConnectionId(dependantConnectionId.toUtf8());
	}
	
	if (connectionRepresentation.Url){
		QUrl url;
		
		if (!GetUrlParamFromRepresentation(url, *connectionRepresentation.Url)){
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
	
	connectionRepresentation.UsageId = usageId;
	connectionRepresentation.DependantConnectionId = dependantServiceConnectionId;
	connectionRepresentation.ServiceTypeName = serviceTypeName;
	
	QUrl url = connectionInfo.GetUrl();
	sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
	if (!GetRepresentationFromUrlParam(url, urlRepresentation)){
		return false;
	}

	connectionRepresentation.Url = urlRepresentation;

	return true;
}


bool GetUrlParamFromRepresentation(QUrl& url, const sdl::agentino::Services::CUrlParameter::V1_0& urlRepresentation)
{
	if (urlRepresentation.Scheme){
		QString scheme = *urlRepresentation.Scheme;
		if (scheme.isEmpty()){
			scheme = "http";
		}
		
		url.setScheme(scheme);
	}
	
	if (urlRepresentation.Host){
		QString host = *urlRepresentation.Host;
		if (host.isEmpty()){
			host = "localhost";
		}
		
		url.setHost(host);
	}
	
	if (urlRepresentation.Port){
		int port = *urlRepresentation.Port;
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
	
	urlRepresentation.Scheme = scheme;
	
	QString host = url.host();
	if (host.isEmpty()){
		host = "localhost";
	}
	
	urlRepresentation.Host = host;
	
	int port = url.port();
	if (port < 0){
		port = 80;
	}
	
	urlRepresentation.Port = port;
	
	return true;
}


} // namespace agentinodata


