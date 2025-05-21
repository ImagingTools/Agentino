import QtQuick 2.12
import Acf 1.0
import agentino 1.0
import agentinoServicesSdl 1.0
import imtguigql 1.0

ServiceController {
	id: root
	
	function startService(serviceId){
		beginStartService(serviceId)
		serviceInput.m_serviceId = serviceId
		startServiceRequestSender.send(root.serviceInput)
	}
	
	function stopService(serviceId){
		beginStopService(serviceId)
		serviceInput.m_serviceId = serviceId
		stopServiceRequestSender.send(root.serviceInput)
	}
	
	property ServiceInput serviceInput: ServiceInput {}
	
	function getHeaders(){
		return {}
	}
	
	property GqlSdlRequestSender startServiceRequestSender : GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoServicesSdlCommandIds.s_startService
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStarted("")
				}
			}
		}
		
		function getHeaders(){
			return root.getHeaders()
		}
	}
	
	property GqlSdlRequestSender stopServiceRequestSender : GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoServicesSdlCommandIds.s_stopService
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					root.serviceStopped("")
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
			root.serviceStatusChanged(serviceId, serviceStatus, data) 
		}
	}
}
