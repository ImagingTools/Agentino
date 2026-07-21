import QtQuick 2.12

/**
	Composition root (Architecture Audit §6.5).
	Shells inject adapters/scope once; views never construct GQL objects.

	Do not use create* methods inside property bindings — each re-evaluation
	creates a new object and can hang the UI (infinite createObject loop).
	Call them once from Component.onCompleted or a plain assignment.
*/
QtObject {
	id: root

	// Eager defaults — no onCompleted createObject (avoids property-change rebinds).
	property var dataScope: DataScope {}
	property var serviceStatusModel: ServiceStatusModel {}

	property Component serviceControllerComponent: Component {
		ServiceController {}
	}

	property Component enrollmentViewModelComponent: Component {
		GqlBasedEnrollmentViewModel {}
	}

	property Component serviceLogViewModelComponent: Component {
		ServiceLogProvider {}
	}

	property Component topologyViewModelComponent: Component {
		TopologyViewModel {}
	}

	property Component typedBackendComponent: Component {
		TypedBackend {}
	}

	function createDataScope(agentId, serviceId) {
		var scope = dataScopeComponent.createObject(root)
		scope.agentId = agentId || ""
		scope.serviceId = serviceId || ""
		return scope
	}

	// Keep Component factory for extra scopes (createDataScope); default dataScope is above.
	property Component dataScopeComponent: Component {
		DataScope {}
	}

	property Component statusModelComponent: Component {
		ServiceStatusModel {}
	}

	function createServiceController(scope) {
		var controller = serviceControllerComponent.createObject(root)
		return controller
	}

	// QG5: swap-readiness — callers can use TypedBackend instead of this object.
	function createTypedBackend() {
		return typedBackendComponent.createObject(root)
	}

	function createEnrollmentViewModel() {
		return enrollmentViewModelComponent.createObject(root)
	}

	function createServiceStatusModel() {
		return statusModelComponent.createObject(root)
	}

	function createServiceLogViewModel(scope) {
		var vm = serviceLogViewModelComponent.createObject(root)
		if (scope)
			vm.dataScope = scope
		return vm
	}

	function createTopologyViewModel(scope) {
		var vm = topologyViewModelComponent.createObject(root)
		if (scope)
			vm.dataScope = scope
		return vm
	}
}
