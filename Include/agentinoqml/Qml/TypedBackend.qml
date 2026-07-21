import QtQuick 2.12

/**
	QG5 — transport-independence proof: same factory surface as AgentinoBackend,
	backed by in-memory ports (no GQL). Views that use only L2 ports work unchanged.

	Uses declared Components + createObject (repo idiom, see ExternPortsDialog) — no
	Qt.createQmlObject, no dynamic function reassignment.

	Do not call create* from property bindings (createObject loop).
*/
QtObject {
	id: root

	property var dataScope: DataScope {}
	property var serviceStatusModel: ServiceStatusModel {}

	property Component scopeComponent: Component {
		DataScope {}
	}

	property Component statusModelComponent: Component {
		ServiceStatusModel {}
	}

	property Component enrollmentViewModelComponent: Component {
		EnrollmentViewModel {}
	}

	property Component topologyViewModelComponent: Component {
		TopologyViewModel {}
	}

	property Component serviceLogViewModelComponent: Component {
		ServiceLogViewModel {}
	}

	property Component serviceControllerComponent: Component {
		ServiceController {}
	}

	function createDataScope(agentId, serviceId) {
		var scope = scopeComponent.createObject(root)
		scope.agentId = agentId || ""
		scope.serviceId = serviceId || ""
		return scope
	}

	function createServiceStatusModel() {
		return statusModelComponent.createObject(root)
	}

	function createEnrollmentViewModel() {
		return enrollmentViewModelComponent.createObject(root)
	}

	function createTopologyViewModel(scope) {
		var vm = topologyViewModelComponent.createObject(root)
		if (scope)
			vm.dataScope = scope
		return vm
	}

	function createServiceLogViewModel(scope) {
		var vm = serviceLogViewModelComponent.createObject(root)
		if (scope)
			vm.dataScope = scope
		return vm
	}

	function createServiceController(scope) {
		return serviceControllerComponent.createObject(root)
	}

	// Already the mock backend — return self (no recursive TypedBackend createObject).
	function createTypedBackend() {
		return root
	}
}
