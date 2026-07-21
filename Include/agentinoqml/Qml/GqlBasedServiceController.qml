import QtQuick 2.12
import Acf 1.0
import agentino 1.0
import agentinoServicesSdl 1.0
import imtguigql 1.0

ServiceController {
	id: root

	// Injected once (QG1); replaces smeared getHeaders() trees.
	property var dataScope: null
	property var serviceStatusModel: ServiceStatusModel {}
	
	function startService(serviceId){
		beginStartService(serviceId)
		startServiceRequestSender.serviceId = serviceId
		startServiceInput.m_serviceId = serviceId
		startServiceRequestSender.send(startServiceInput)
	}
	
	function stopService(serviceId){
		beginStopService(serviceId)
		stopServiceRequestSender.serviceId = serviceId
		stopServiceInput.m_serviceId = serviceId
		stopServiceRequestSender.send(stopServiceInput)
	}
	
	property ServiceInput startServiceInput: ServiceInput {}
	property ServiceInput stopServiceInput: ServiceInput {}

	function normalizeServiceStatus(status){
		// Map through single ServiceStatusModel then back to SDL enum tokens.
		// unknown/failed/crashed stay s_Undefined so Start/Stop stay disabled.
		let n = serviceStatusModel.normalize(status)
		if (n === serviceStatusModel.running) return ServiceStatus.s_Running
		if (n === serviceStatusModel.starting) return ServiceStatus.s_Starting
		if (n === serviceStatusModel.stopping) return ServiceStatus.s_Stopping
		if (n === serviceStatusModel.stopped) return ServiceStatus.s_NotRunning
		if (n === serviceStatusModel.unknown
					|| n === serviceStatusModel.failed
					|| n === serviceStatusModel.crashed)
			return ServiceStatus.s_Undefined
		return status
	}
	
	function getHeaders(){
		if (dataScope === null)
			return {}
		let headers = {}
		if (dataScope.agentId && dataScope.agentId.length > 0)
			headers.clientid = dataScope.agentId
		if (dataScope.serviceId && dataScope.serviceId.length > 0)
			headers.serviceid = dataScope.serviceId
		return headers
	}
	
	property GqlSdlRequestSender startServiceRequestSender : GqlSdlRequestSender {
		property string serviceId: ""
		requestType: 1
		gqlCommandId: "StartService"
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStatusChanged(root.startServiceRequestSender.serviceId, root.normalizeServiceStatus(m_status), this)
					root.serviceStarted(root.startServiceRequestSender.serviceId)
				}
			}
		}
		
		function getHeaders(){
			return root.getHeaders()
		}
	}
	
	property GqlSdlRequestSender stopServiceRequestSender : GqlSdlRequestSender {
		property string serviceId: ""
		requestType: 1
		gqlCommandId: "StopService"
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStatusChanged(root.stopServiceRequestSender.serviceId, root.normalizeServiceStatus(m_status), this)
					root.serviceStopped(root.stopServiceRequestSender.serviceId)
				}
			}
		}
		
		function getHeaders(){
			return root.getHeaders()
		}
	}

	property SubscriptionClient serviceStateSubscription : SubscriptionClient {
		gqlCommandId: "OnServiceStatusChanged"
		onMessageReceived: {
			let serviceId = data.getData("serviceid")
			let serviceStatus = data.getData(ServiceStatus.s_Key)
			root.serviceStatusChanged(serviceId, root.normalizeServiceStatus(serviceStatus), data)
		}
	}
}
