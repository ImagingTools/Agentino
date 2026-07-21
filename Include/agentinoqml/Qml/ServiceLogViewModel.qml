import QtQuick 2.12

/**
	L2 port: service log stream for a DataScope (Architecture §6.4).
*/
QtObject {
	id: root

	property var dataScope: null
	property var logModel: null
	property bool loading: false
	property string errorMessage: ""

	signal refresh(string serviceId)
	signal logUpdated()

	function setLogModel(model) {
		root.logModel = model
		logUpdated()
	}
}
