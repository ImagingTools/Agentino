import QtQuick 2.12
import Acf 1.0
import agentino 1.0
import agentinoServicesSdl 1.0
import imtguigql 1.0

ServiceController {
	id: root
	
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
		switch (status){
			case "RUNNING": return ServiceStatus.s_Running
			case "NOT_RUNNING": return ServiceStatus.s_NotRunning
			case "STARTING": return ServiceStatus.s_Starting
			case "STOPPING": return ServiceStatus.s_Stopping
			case "UNDEFINED":
			case "RUNNING_IMPOSSIBLE": return ServiceStatus.s_Undefined
		}
		return status
	}
	
	function getHeaders(){
		return {}
	}
	
	property GqlSdlRequestSender startServiceRequestSender : GqlSdlRequestSender {
		property string serviceId: ""
		requestType: 1
		gqlCommandId: AgentinoServicesSdlCommandIds.s_startService
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStatusChanged(startServiceRequestSender.serviceId, root.normalizeServiceStatus(m_status), this)
					root.serviceStarted(startServiceRequestSender.serviceId)
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
		gqlCommandId: AgentinoServicesSdlCommandIds.s_stopService
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStatusChanged(stopServiceRequestSender.serviceId, root.normalizeServiceStatus(m_status), this)
					root.serviceStopped(stopServiceRequestSender.serviceId)
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
