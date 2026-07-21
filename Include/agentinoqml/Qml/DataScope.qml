import QtQuick 2.12

/**
	Typed scope injected once per data source (QG1).
	Replaces getHeaders() { clientid, serviceid } scattered through the tree.
*/
QtObject {
	id: root

	property string agentId: ""
	property string serviceId: ""

	function isEmpty() {
		return agentId.length === 0 && serviceId.length === 0
	}
}
