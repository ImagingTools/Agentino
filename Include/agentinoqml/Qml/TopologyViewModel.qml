import QtQuick 2.12
import Acf 1.0
import imtguigql 1.0
import agentino 1.0
import agentinoTopologySdl 1.0
import agentinoServicesSdl 1.0

/**
	L2/L3 Topology port + GQL adapter (Architecture §6.4 / QG2).
	View binds only to properties/signals — no gqlCommandId in the view file.
*/
QtObject {
	id: root

	property var dataScope: DataScope {}
	property var serviceStatusModel: ServiceStatusModel {}
	property var objectsModel: null
	property bool loading: false
	property string errorMessage: ""
	// While SaveTopology is in flight, ignore OnTopologyChanged reloads — they race the
	// server write and replace the canvas with stale GetTopology data (nodes snap back).
	property bool saveInProgress: false

	signal topologyLoaded(var topologySdlObject)
	signal topologySaved()
	signal reloadRequested()
	signal serviceStatusMessage(var data)
	signal collectionChanged()
	signal agentStatusChanged()

	function normalizeServiceStatus(status) {
		if (serviceStatusModel !== null) {
			let n = serviceStatusModel.normalize(status)
			if (n === serviceStatusModel.running) return "RUNNING"
			if (n === serviceStatusModel.starting) return "STARTING"
			if (n === serviceStatusModel.stopping) return "STOPPING"
			if (n === serviceStatusModel.failed || n === serviceStatusModel.crashed) return "RUNNING_IMPOSSIBLE"
			if (n === serviceStatusModel.stopped) return "NOT_RUNNING"
			if (n === serviceStatusModel.unknown) return "UNDEFINED"
		}
		return "UNDEFINED"
	}

	function loadTopology() {
		if (!getTopologyRequestSender.gqlCommandId){
			console.error("GetTopology: gqlCommandId is empty")
			root.loading = false
			return
		}
		root.loading = true
		getTopologyRequestSender.send()
	}

	// SaveTopology only persists coordinates (see CTopologyControllerComp::OnSaveTopology,
	// which reads id/x/y and nothing else) — the input is the minimal ServiceCoordinateInput,
	// not the full Service type. SDL-generated "m_services" is a QVariant that only accepts
	// a ServiceCoordinateInputObjectList* (built via emplaceServices()/
	// createServicesArrayElement()/addElement()); plain JS values are silently rejected by
	// the generated setter, so services would be sent as null on the wire.
	function buildTopologyInput(model) {
		let topologyInput = topologyInputComp.createObject(root)
		topologyInput.emplaceServices()
		for (let i = 0; i < model.count; ++i){
			let item = model.get(i).item
			if (!item){
				continue
			}
			let coordinate = topologyInput.createServicesArrayElement()
			coordinate.m_id = item.m_id
			coordinate.m_x = item.m_x
			coordinate.m_y = item.m_y
			topologyInput.m_services.addElement(coordinate)
		}
		return topologyInput
	}

	function saveTopology(model) {
		if (!saveModelRequestSender.gqlCommandId){
			console.error("SaveTopology: gqlCommandId is empty")
			return
		}
		if (!model){
			return
		}
		root.saveInProgress = true
		let topologyInput = buildTopologyInput(model)
		saveModelRequestSender.send(topologyInput)
		topologyInput.destroy()
	}

	property Component topologyInputComp: Component { SaveTopologyInput {} }

	function requestReload() {
		if (root.saveInProgress){
			return
		}
		reloadRequested()
		loadTopology()
	}

	// --- L3 transport (must stay out of L1 views) ---

	property SubscriptionClient topologySubscriptionClient: SubscriptionClient {
		gqlCommandId: "OnTopologyChanged"
		onMessageReceived: root.requestReload()
	}

	property SubscriptionClient servicesCollectionSubscriptionClient: SubscriptionClient {
		gqlCommandId: "OnServicesCollectionChanged"
		onMessageReceived: {
			root.collectionChanged()
			root.requestReload()
		}
	}

	property SubscriptionClient agentStatusSubscriptionClient: SubscriptionClient {
		gqlCommandId: "OnAgentStatusChanged"
		onMessageReceived: {
			root.agentStatusChanged()
			root.requestReload()
		}
	}

	property SubscriptionClient serviceStatusSubscriptionClient: SubscriptionClient {
		gqlCommandId: "OnServiceStatusChanged"
		onMessageReceived: root.serviceStatusMessage(data)
	}

	property GqlSdlRequestSender saveModelRequestSender: GqlSdlRequestSender {
		requestType: 1
		// Wire names (same as AgentinoTopologySdlCommandIds) — string literals so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "SaveTopology"
		sdlObjectComp: Component {
			SaveTopologyResponse {
				onFinished: {
					root.saveInProgress = false
					root.topologySaved()
					// Local scheme already has the dragged positions — no GetTopology reload.
				}
			}
		}
		// status -1 = error; always clear the save gate so later OnTopologyChanged works.
		onFinished: {
			root.saveInProgress = false
			if (status < 0){
				console.error("SaveTopology failed")
			}
		}
	}

	property GqlSdlRequestSender getTopologyRequestSender: GqlSdlRequestSender {
		gqlCommandId: "GetTopology"
		sdlObjectComp: Component {
			Topology {
				onFinished: {
					root.loading = false
					root.topologyLoaded(this)
				}
			}
		}
	}
}
