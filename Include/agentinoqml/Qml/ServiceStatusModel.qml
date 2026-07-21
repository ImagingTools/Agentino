import QtQuick 2.12

/**
	Single owner of service status enum ↔ display mapping (QG0).
	Accepts both legacy SS_* / RUNNING strings and new FSM names.

	"undefined" is not "stopped": when the agent is disconnected the service
	status is unknown and Start/Stop must stay disabled.
*/
QtObject {
	id: root

	readonly property string stopped: "Stopped"
	readonly property string starting: "Starting"
	readonly property string running: "Running"
	readonly property string stopping: "Stopping"
	readonly property string crashed: "Crashed"
	readonly property string failed: "Failed"
	readonly property string unknown: "Unknown"

	function normalize(status) {
		if (status === undefined || status === null || status === "")
			return root.stopped
		// Collapse camelCase / snake_case / enum forms: notRunning, NOT_RUNNING, SS_NOT_RUNNING
		var s = ("" + status).toUpperCase().replace(/_/g, "")
		if (s === "RUNNING" || s === "SSRUNNING" || s === "SRUNNING")
			return root.running
		if (s === "STARTING" || s === "SSSTARTING")
			return root.starting
		if (s === "STOPPING" || s === "SSSTOPPING")
			return root.stopping
		if (s === "CRASHED")
			return root.crashed
		if (s === "FAILED" || s === "RUNNINGIMPOSSIBLE" || s === "SSRUNNINGIMPOSSIBLE")
			return root.failed
		if (s === "UNDEFINED" || s === "SSUNDEFINED")
			return root.unknown
		if (s === "STOPPED" || s === "NOTRUNNING" || s === "SSNOTRUNNING")
			return root.stopped
		// Already canonical display labels
		if (status === root.running || status === root.starting || status === root.stopping
				|| status === root.crashed || status === root.failed || status === root.stopped
				|| status === root.unknown)
			return status
		return root.stopped
	}

	function isRunning(status) {
		return normalize(status) === root.running
	}

	function isStopped(status) {
		return normalize(status) === root.stopped
	}

	// True only when Start is meaningful (known stopped process, agent reachable).
	function isStartable(status) {
		return normalize(status) === root.stopped
	}

	// True only when Stop is meaningful (known running process).
	function isStoppable(status) {
		return normalize(status) === root.running
	}

	function isTransitional(status) {
		var n = normalize(status)
		return n === root.starting || n === root.stopping
	}

	function isFailed(status) {
		var n = normalize(status)
		return n === root.failed || n === root.crashed
	}

	function isUnknown(status) {
		return normalize(status) === root.unknown
	}

	function displayLabel(status) {
		return normalize(status)
	}

	function displayColor(status) {
		var n = normalize(status)
		if (n === root.running)
			return "#2e7d32"
		if (n === root.starting || n === root.stopping)
			return "#f9a825"
		if (n === root.crashed || n === root.failed || n === root.unknown)
			return "#c62828"
		return "#757575"
	}
}
