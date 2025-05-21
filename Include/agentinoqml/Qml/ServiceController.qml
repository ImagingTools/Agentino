import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0

QtObject {
	id: root
	
	property CommandsController commandsController: null
	
	signal beginStartService(string serviceId)
	signal beginStopService(string serviceId)
	signal serviceStarted(string serviceId)
	signal serviceStopped(string serviceId)
	signal startServiceFailed(string serviceId, string message)
	signal stopServiceFailed(string serviceId, string message)
	
	// "RUNNING" "NOT_RUNNING"
	signal serviceStatusChanged(string serviceId, string status, var params)

	function startService(serviceId){
		beginStartService(serviceId)
		serviceStarted(serviceId)
	}
	
	function stopService(serviceId){
		beginStopService(serviceId)
		serviceStopped(serviceId)
	}
}


