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
	// Current view (way of grouping); empty for the default view grouping services by agents.
	property string viewId: ""
	// False until GetTopologyViews succeeds (e.g. the agent app has no views).
	property bool viewsSupported: false
	// Set when the server answered that views are not supported; transport failures do not set it.
	property bool viewsUnsupported: false

	signal topologyLoaded(var topologySdlObject)
	signal topologySaved()
	signal reloadRequested()
	signal serviceStatusMessage(var data)
	signal collectionChanged()
	signal agentStatusChanged()
	signal groupCreated(string groupId)
	signal groupOperationFailed(string message)
	// views: [{id, name}] of the user-defined views (the default view is not listed).
	signal viewsLoaded(var views)
	signal viewCreated(string viewId)
	signal viewRemoved(string viewId)

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
		// One request at a time: responses are not tied to requests, so a response for a
		// previous view could replace the current one. Later loads are merged into one.
		if (root.loading){
			root.reloadPending = true
			return
		}
		root.loading = true
		root.reloadPending = false
		getTopologyInput.m_viewId = root.viewId
		getTopologyRequestSender.requestedViewId = root.viewId
		getTopologyRequestSender.send(getTopologyInput)
	}

	property bool reloadPending: false
	property GetTopologyInput getTopologyInput: GetTopologyInput {}

	// SaveTopology only persists coordinates (see CTopologyControllerComp::OnSaveTopology,
	// which reads id/x/y and nothing else) — the input is the minimal ServiceCoordinateInput,
	// not the full Service type. SDL-generated "m_services" is a QVariant that only accepts
	// a ServiceCoordinateInputObjectList* (built via emplaceServices()/
	// createServicesArrayElement()/addElement()); plain JS values are silently rejected by
	// the generated setter, so services would be sent as null on the wire.
	function buildTopologyInput(model) {
		let topologyInput = topologyInputComp.createObject(root)
		topologyInput.m_viewId = root.viewId
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

	// --- Topology groups (composite nodes) ---

	// parentId: group containing the new group, empty for the root level.
	function createGroup(name, parentId, x, y) {
		createGroupInput.m_viewId = root.viewId
		createGroupInput.m_name = "" + name
		createGroupInput.m_parentId = parentId ? "" + parentId : ""
		createGroupInput.m_x = x
		createGroupInput.m_y = y
		createGroupRequestSender.send(createGroupInput)
	}

	// parentId: target group; empty returns the nodes to their default place
	// (in the default view a service to its agent, otherwise the root level).
	function moveNodes(nodeIds, parentId) {
		moveNodesInput.m_viewId = root.viewId
		moveNodesInput.m_nodeIds = nodeIds
		moveNodesInput.m_parentId = parentId ? "" + parentId : ""
		moveNodesRequestSender.send(moveNodesInput)
	}

	function removeGroup(groupId) {
		removeGroupInput.m_id = "" + groupId
		removeGroupRequestSender.send(removeGroupInput)
	}

	function renameGroup(groupId, name) {
		renameGroupInput.m_id = "" + groupId
		renameGroupInput.m_name = "" + name
		renameGroupRequestSender.send(renameGroupInput)
	}

	// The service name is owned by the agent: the service is renamed the same way the service
	// editor saves it (GetService, then UpdateService with the new name, forwarded to the agent).
	// agentId: owning agent (clientid header); empty in the agent app.
	function renameService(serviceId, agentId, name) {
		let controller = serviceRenameControllerComp.createObject(root)
		if (!controller){
			root.groupOperationFailed(qsTr("Unable to rename the service"))
			return
		}
		controller.agentId = agentId ? "" + agentId : ""
		controller.newName = "" + name
		controller.documentId = "" + serviceId
		controller.updateDocumentModel()
	}

	property Component serviceRenameControllerComp: Component {
		ServiceDocumentDataController {
			id: renameController

			property string agentId: ""
			property string newName: ""
			property bool saving: false

			// Destroyed after the request finished, not from inside its handler.
			function release() {
				Qt.callLater(function(){
					renameController.destroy()
				})
			}

			function getHeaders() {
				let headers = {}
				if (renameController.agentId !== ""){
					headers["clientid"] = renameController.agentId
				}
				return headers
			}

			onModelChanged: {
				if (renameController.saving || !renameController.documentModel){
					return
				}
				renameController.saving = true
				renameController.documentModel.m_name = renameController.newName
				renameController.saveDocument()
			}

			onSaved: {
				root.requestReload()
				renameController.release()
			}

			// The base controller already shows the error message.
			onError: {
				renameController.release()
			}
		}
	}

	// --- Topology views (ways of grouping) ---

	function loadViews() {
		if (root.viewsUnsupported){
			return
		}
		getViewsRequestSender.send()
	}

	function createView(name) {
		createViewInput.m_name = "" + name
		createViewRequestSender.send(createViewInput)
	}

	function removeView(viewId) {
		removeViewInput.m_id = "" + viewId
		removeViewRequestSender.removedViewId = "" + viewId
		removeViewRequestSender.send(removeViewInput)
	}

	property CreateTopologyViewInput createViewInput: CreateTopologyViewInput {}
	property RemoveTopologyViewInput removeViewInput: RemoveTopologyViewInput {}

	property GqlSdlRequestSender getViewsRequestSender: GqlSdlRequestSender {
		gqlCommandId: "GetTopologyViews"
		sdlObjectComp: Component {
			TopologyViewList {
				onFinished: {
					let views = []
					if (m_views){
						for (let i = 0; i < m_views.count; ++i){
							let item = m_views.get(i).item
							if (item){
								views.push({"id": item.m_id, "name": item.m_name})
							}
						}
					}
					root.viewsSupported = true
					root.viewsLoaded(views)
				}
			}
		}
		// The agent app has no views: no popup, the views panel stays hidden. Only an answer of the
		// server (state "Ready" with GraphQL errors) means "unsupported"; a network error is retried later.
		function onError(message, type) {
			if (state === "Ready"){
				root.viewsUnsupported = true
				root.viewsSupported = false
			}
			finished(-1)
		}
	}

	property GqlSdlRequestSender createViewRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "CreateTopologyView"
		sdlObjectComp: Component {
			CreateTopologyViewResponse {
				onFinished: {
					root.viewCreated(m_id)
					root.loadViews()
				}
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeViewRequestSender: GqlSdlRequestSender {
		property string removedViewId: ""
		requestType: 1
		gqlCommandId: "RemoveTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: {
					if (m_successful === true){
						root.viewRemoved(root.removeViewRequestSender.removedViewId)
					}
					root.loadViews()
				}
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property CreateTopologyGroupInput createGroupInput: CreateTopologyGroupInput {}
	property MoveTopologyNodesInput moveNodesInput: MoveTopologyNodesInput {}
	property RemoveTopologyGroupInput removeGroupInput: RemoveTopologyGroupInput {}
	property RenameTopologyGroupInput renameGroupInput: RenameTopologyGroupInput {}

	property GqlSdlRequestSender renameGroupRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "RenameTopologyGroup"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.requestReload()
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender createGroupRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "CreateTopologyGroup"
		sdlObjectComp: Component {
			CreateTopologyGroupResponse {
				onFinished: {
					root.groupCreated(m_id)
					root.requestReload()
				}
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender moveNodesRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "MoveTopologyNodes"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.requestReload()
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeGroupRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: "RemoveTopologyGroup"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.requestReload()
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

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
		onMessageReceived: {
			// Views are stored in the topology collection as well (added/removed by other clients).
			root.loadViews()
			root.requestReload()
		}
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
		// View of the request in flight; the response is dropped if another view was chosen meanwhile.
		property string requestedViewId: ""
		gqlCommandId: "GetTopology"
		sdlObjectComp: Component {
			Topology {
				onFinished: {
					root.loading = false
					if (root.reloadPending || root.getTopologyRequestSender.requestedViewId !== root.viewId){
						root.loadTopology()
						return
					}
					root.topologyLoaded(this)
				}
			}
		}
		// status -1 = error: release the single-request gate so later loads are sent.
		onFinished: {
			if (status < 0){
				root.loading = false
				if (root.reloadPending){
					root.loadTopology()
				}
			}
		}
	}
}
