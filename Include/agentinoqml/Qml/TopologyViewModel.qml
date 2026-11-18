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
	// views: [{id, name, nodeNames}] of the user-defined views (the default view is not listed);
	// nodeNames: names of the services and groups of the view, agentsViewNodeNames - the same for the default view.
	signal viewsLoaded(var views, var agentsViewNodeNames)
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
	// positions: [{id, x, y}]
	function buildTopologyInput(viewId, positions) {
		let topologyInput = topologyInputComp.createObject(root)
		topologyInput.m_viewId = viewId
		topologyInput.emplaceServices()
		for (let i = 0; i < positions.length; ++i){
			let coordinate = topologyInput.createServicesArrayElement()
			coordinate.m_id = positions[i].id
			coordinate.m_x = positions[i].x
			coordinate.m_y = positions[i].y
			topologyInput.m_services.addElement(coordinate)
		}
		return topologyInput
	}

	// [{id, x, y}] of all nodes of the scheme model.
	function modelPositions(model) {
		let positions = []
		if (!model){
			return positions
		}
		for (let i = 0; i < model.count; ++i){
			let item = model.get(i).item
			if (item){
				positions.push({"id": item.m_id, "x": item.m_x, "y": item.m_y})
			}
		}
		return positions
	}

	property Component topologyInputComp: Component { SaveTopologyInput {} }

	// --- Undo / Redo ---
	// Every change of the topology is applied on the server at once. An action of the history knows
	// how to apply it again (redo) and how to revert it (undo) with the same server operations.
	// The operations run one after another: the request senders and their inputs are shared, and
	// an undo must not overtake the change it reverts. Physical removal of services and Start/Stop
	// are not in the history.

	readonly property int maxHistorySteps: 50
	property var undoStack: []
	property var redoStack: []
	property int undoCount: 0
	property int redoCount: 0
	// An undo/redo step is running: it has left one stack and is not in the other one yet.
	property bool historyActionPending: false
	property var operationQueue: []
	property bool operationInProgress: false
	// Undo/Redo wait for the running changes, otherwise the history would not match the server.
	readonly property bool canUndo: root.undoCount > 0 && !root.operationInProgress
	readonly property bool canRedo: root.redoCount > 0 && !root.operationInProgress

	function updateHistoryCounts() {
		root.undoCount = root.undoStack.length
		root.redoCount = root.redoStack.length
	}

	// The history no longer matches the server (e.g. the topology was changed by another client).
	function clearHistory() {
		root.historyActionPending = false
		root.undoStack = []
		root.redoStack = []
		root.updateHistoryCounts()
	}

	// A change that got no answer (e.g. a request path without an error callback) must not block the
	// queue for good: it is abandoned after this time and the history is cleared.
	property int operationTimeout: 60000
	property var currentOperation: null

	property Timer operationTimer: Timer {
		interval: root.operationTimeout
		repeat: false
		onTriggered: root.abandonCurrentOperation()
	}

	function abandonCurrentOperation() {
		let operation = root.currentOperation
		if (!operation || operation.isFinished){
			return
		}
		operation.isAborted = true
		root.saveInProgress = false
		root.clearHistory()
		root.groupOperationFailed(qsTr("The topology change has not been completed. Check the topology and repeat the change if necessary."))
		operation.finish()
		root.requestReload()
	}

	// run: function(finish, operation) calling finish() once the operation is completed;
	// operation.isAborted is set if the operation was abandoned (its late result is ignored).
	function enqueueOperation(run) {
		let queue = root.operationQueue.slice()
		queue.push(run)
		root.operationQueue = queue
		root.runNextOperation()
	}

	function runNextOperation() {
		if (root.operationInProgress || root.operationQueue.length === 0){
			return
		}
		let queue = root.operationQueue.slice()
		let run = queue.shift()
		root.operationQueue = queue
		root.operationInProgress = true

		let operation = {"isFinished": false, "isAborted": false}
		operation.finish = function(){
			if (operation.isFinished){
				return
			}
			operation.isFinished = true
			root.operationTimer.stop()
			root.currentOperation = null
			root.operationInProgress = false
			Qt.callLater(root.runNextOperation)
		}
		root.currentOperation = operation
		root.operationTimer.restart()
		run(operation.finish, operation)
	}

	// Runs the steps (function(next) calling next(ok)) one after another; stops at the first failure.
	function runSteps(steps, done) {
		let index = 0
		let next = function(ok){
			if (ok === false){
				done(false)
				return
			}
			if (index >= steps.length){
				done(true)
				return
			}
			let step = steps[index]
			++index
			step(next)
		}
		next(true)
	}

	// action: {run(done), undo(done), redo(done), reloadOnRun, viewsChanged}; done(ok).
	// recordable === false applies the change without a history step.
	function performAction(action, recordable) {
		root.enqueueOperation(function(finish, operation){
			action.run(function(ok){
				if (operation.isAborted){
					return
				}
				if (ok && recordable !== false){
					let stack = root.undoStack.slice()
					stack.push(action)
					while (stack.length > root.maxHistorySteps){
						stack.shift()
					}
					root.undoStack = stack
					root.redoStack = []
					root.updateHistoryCounts()
				}
				root.finishAction(action, ok, action.reloadOnRun !== false || !ok)
				finish()
			})
		})
	}

	function undo() {
		if (!root.canUndo){
			return
		}
		let stack = root.undoStack.slice()
		let action = stack.pop()
		root.historyActionPending = true
		root.undoStack = stack
		root.updateHistoryCounts()

		root.enqueueOperation(function(finish, operation){
			action.undo(function(ok){
				if (operation.isAborted){
					return
				}
				if (ok){
					let redoStack = root.redoStack.slice()
					redoStack.push(action)
					root.redoStack = redoStack
					root.updateHistoryCounts()
				}
				else{
					root.clearHistory()
				}
				root.historyActionPending = false
				root.finishAction(action, ok, true)
				finish()
			})
		})
	}

	function redo() {
		if (!root.canRedo){
			return
		}
		let stack = root.redoStack.slice()
		let action = stack.pop()
		root.historyActionPending = true
		root.redoStack = stack
		root.updateHistoryCounts()

		root.enqueueOperation(function(finish, operation){
			action.redo(function(ok){
				if (operation.isAborted){
					return
				}
				if (ok){
					let undoStack = root.undoStack.slice()
					undoStack.push(action)
					root.undoStack = undoStack
					root.updateHistoryCounts()
				}
				else{
					root.clearHistory()
				}
				root.historyActionPending = false
				root.finishAction(action, ok, true)
				finish()
			})
		})
	}

	function finishAction(action, ok, reload) {
		if (action.viewsChanged === true){
			root.loadViews()
		}
		if (reload){
			root.requestReload()
		}
	}

	// --- Topology changes (recorded in the history) ---

	// Saves the layout after tiles were moved on the shown level. The whole level is saved (new tiles
	// get positions on the client); before/after: [{id, x, y}] of the moved tiles for undo/redo.
	function saveTopology(model, before, after) {
		if (!saveModelRequestSender.gqlCommandId){
			console.error("SaveTopology: gqlCommandId is empty")
			return
		}
		if (!model){
			return
		}
		let viewId = root.viewId
		let positions = root.modelPositions(model)
		let movedBefore = before ? before : []
		let movedAfter = after ? after : []
		let action = {
			// The scheme already shows the new positions.
			"reloadOnRun": false,
			"run": function(done){ root.opSavePositions(viewId, positions, done) },
			"undo": function(done){ root.opSavePositions(viewId, movedBefore, done) },
			"redo": function(done){ root.opSavePositions(viewId, movedAfter, done) }
		}
		root.performAction(action, movedBefore.length > 0)
	}

	// parentId: group containing the new group, empty for the root level.
	function createGroup(name, parentId, x, y) {
		let viewId = root.viewId
		let groupName = "" + name
		let groupParentId = parentId ? "" + parentId : ""
		let action = {"groupId": ""}
		action.run = function(done){
			root.opCreateGroup(viewId, "", groupName, groupParentId, x, y, function(ok, groupId){
				if (ok){
					action.groupId = "" + groupId
					root.groupCreated(action.groupId)
				}
				done(ok)
			})
		}
		// The group is empty again: the later changes are undone before.
		action.undo = function(done){ root.opRemoveGroup(action.groupId, done) }
		action.redo = function(done){ root.opCreateGroup(viewId, action.groupId, groupName, groupParentId, x, y, done) }
		root.performAction(action)
	}

	// parentId: target group; empty returns the nodes to their default place
	// (in the default view a service to its agent, otherwise the root level).
	// oldParentIds: {nodeId: previous group of the node or ""} to undo the move.
	function moveNodes(nodeIds, parentId, oldParentIds) {
		let viewId = root.viewId
		let ids = nodeIds.slice()
		let targetId = parentId ? "" + parentId : ""
		let previousParents = oldParentIds ? oldParentIds : {}
		root.performAction({
			"run": function(done){ root.opMoveNodes(viewId, ids, targetId, done) },
			"undo": function(done){
				// Nodes are returned to their previous groups, one request per group.
				let byParent = {}
				for (let i = 0; i < ids.length; ++i){
					let previousId = previousParents[ids[i]] ? "" + previousParents[ids[i]] : ""
					if (!byParent[previousId]){
						byParent[previousId] = []
					}
					byParent[previousId].push(ids[i])
				}
				let steps = []
				for (let previousId in byParent){
					let groupIds = byParent[previousId]
					let groupId = previousId
					steps.push(function(next){ root.opMoveNodes(viewId, groupIds, groupId, next) })
				}
				root.runSteps(steps, done)
			},
			"redo": function(done){ root.opMoveNodes(viewId, ids, targetId, done) }
		})
	}

	// info: {name, parentId, x, y, memberIds} of the group, used to recreate it on undo.
	function removeGroup(groupId, info) {
		let viewId = root.viewId
		let id = "" + groupId
		let groupInfo = info ? info : {"name": "", "parentId": "", "x": 0, "y": 0, "memberIds": []}
		root.performAction({
			"run": function(done){ root.opRemoveGroup(id, done) },
			"undo": function(done){
				// The group is recreated with the same ID, its content is moved back into it.
				let steps = [function(next){
					root.opCreateGroup(viewId, id, groupInfo.name, groupInfo.parentId, groupInfo.x, groupInfo.y, next)
				}]
				if (groupInfo.memberIds.length > 0){
					steps.push(function(next){ root.opMoveNodes(viewId, groupInfo.memberIds, id, next) })
				}
				root.runSteps(steps, done)
			},
			"redo": function(done){ root.opRemoveGroup(id, done) }
		})
	}

	function renameGroup(groupId, name, oldName) {
		let id = "" + groupId
		let newName = "" + name
		let previousName = "" + oldName
		root.performAction({
			"viewsChanged": true,
			"run": function(done){ root.opRenameGroup(id, newName, done) },
			"undo": function(done){ root.opRenameGroup(id, previousName, done) },
			"redo": function(done){ root.opRenameGroup(id, newName, done) }
		}, previousName !== "")
	}

	// Removes the services from the current user-defined view only; the services are not deleted.
	// oldParentId: group of the view containing the services (empty for the root level), used on undo.
	function removeFromView(nodeIds, oldParentId) {
		if (root.viewId === ""){
			return
		}
		let viewId = root.viewId
		let ids = nodeIds.slice()
		let previousParentId = oldParentId ? "" + oldParentId : ""
		root.performAction({
			"viewsChanged": true,
			"run": function(done){ root.opRemoveFromView(viewId, ids, done) },
			"undo": function(done){ root.opAddToView(viewId, ids, previousParentId, done) },
			"redo": function(done){ root.opRemoveFromView(viewId, ids, done) }
		})
	}

	// Adds services (not yet in the current user-defined view) to it.
	// parentId: group of the view receiving the services; empty for the root level.
	function addToView(nodeIds, parentId) {
		if (root.viewId === ""){
			return
		}
		let viewId = root.viewId
		let ids = nodeIds.slice()
		let targetId = parentId ? "" + parentId : ""
		root.performAction({
			"viewsChanged": true,
			"run": function(done){ root.opAddToView(viewId, ids, targetId, done) },
			"undo": function(done){ root.opRemoveFromView(viewId, ids, done) },
			"redo": function(done){ root.opAddToView(viewId, ids, targetId, done) }
		})
	}

	// The service name is owned by the agent (see opRenameService).
	function renameService(serviceId, agentId, name, oldName) {
		let id = "" + serviceId
		let agent = agentId ? "" + agentId : ""
		let newName = "" + name
		let previousName = "" + oldName
		root.performAction({
			"viewsChanged": true,
			"run": function(done){ root.opRenameService(id, agent, newName, done) },
			"undo": function(done){ root.opRenameService(id, agent, previousName, done) },
			"redo": function(done){ root.opRenameService(id, agent, newName, done) }
		}, previousName !== "")
	}

	function createView(name) {
		let viewName = "" + name
		let action = {"viewsChanged": true, "createdViewId": ""}
		action.run = function(done){
			root.opCreateView(viewName, function(ok, viewId){
				if (ok){
					action.createdViewId = "" + viewId
					root.viewCreated(action.createdViewId)
				}
				done(ok)
			})
		}
		// A removed view is kept on the server and restored with its ID.
		action.undo = function(done){ root.opRemoveView(action.createdViewId, done) }
		action.redo = function(done){ root.opRestoreView(action.createdViewId, done) }
		root.performAction(action)
	}

	function removeView(viewId) {
		let id = "" + viewId
		root.performAction({
			"viewsChanged": true,
			"run": function(done){ root.opRemoveView(id, done) },
			"undo": function(done){ root.opRestoreView(id, done) },
			"redo": function(done){ root.opRemoveView(id, done) }
		})
	}

	// --- Server operations; done(ok, result) is called once ---

	function completeOperation(sender, ok, result) {
		let callback = sender.callback
		sender.callback = null
		if (callback){
			callback(ok, result)
		}
	}

	function opSavePositions(viewId, positions, done) {
		if (positions.length === 0){
			done(true)
			return
		}
		root.saveInProgress = true
		let topologyInput = buildTopologyInput(viewId, positions)
		saveModelRequestSender.callback = function(ok){
			root.saveInProgress = false
			if (ok){
				root.topologySaved()
			}
			done(ok)
		}
		saveModelRequestSender.send(topologyInput)
		topologyInput.destroy()
	}

	// groupId: ID of the new group; empty generates a new ID. done(ok, groupId).
	function opCreateGroup(viewId, groupId, name, parentId, x, y, done) {
		createGroupInput.m_viewId = viewId
		createGroupInput.m_id = groupId ? "" + groupId : ""
		createGroupInput.m_name = "" + name
		createGroupInput.m_parentId = parentId ? "" + parentId : ""
		createGroupInput.m_x = x
		createGroupInput.m_y = y
		createGroupRequestSender.callback = done
		createGroupRequestSender.send(createGroupInput)
	}

	function opMoveNodes(viewId, nodeIds, parentId, done) {
		moveNodesInput.m_viewId = viewId
		moveNodesInput.m_nodeIds = nodeIds
		moveNodesInput.m_parentId = parentId ? "" + parentId : ""
		moveNodesRequestSender.callback = done
		moveNodesRequestSender.send(moveNodesInput)
	}

	function opRemoveGroup(groupId, done) {
		removeGroupInput.m_id = "" + groupId
		removeGroupRequestSender.callback = done
		removeGroupRequestSender.send(removeGroupInput)
	}

	function opRenameGroup(groupId, name, done) {
		renameGroupInput.m_id = "" + groupId
		renameGroupInput.m_name = "" + name
		renameGroupRequestSender.callback = done
		renameGroupRequestSender.send(renameGroupInput)
	}

	function opRemoveFromView(viewId, nodeIds, done) {
		removeFromViewInput.m_viewId = viewId
		removeFromViewInput.m_nodeIds = nodeIds
		removeFromViewRequestSender.callback = done
		removeFromViewRequestSender.send(removeFromViewInput)
	}

	function opAddToView(viewId, nodeIds, parentId, done) {
		addToViewInput.m_viewId = viewId
		addToViewInput.m_nodeIds = nodeIds
		addToViewInput.m_parentId = parentId ? "" + parentId : ""
		addToViewRequestSender.callback = done
		addToViewRequestSender.send(addToViewInput)
	}

	// done(ok, viewId)
	function opCreateView(name, done) {
		createViewInput.m_name = "" + name
		createViewRequestSender.callback = done
		createViewRequestSender.send(createViewInput)
	}

	// The view is kept on the server (marked as removed) and can be restored.
	function opRemoveView(viewId, done) {
		removeViewInput.m_id = "" + viewId
		removeViewRequestSender.callback = function(ok){
			if (ok){
				root.viewRemoved(viewId)
			}
			done(ok)
		}
		removeViewRequestSender.send(removeViewInput)
	}

	function opRestoreView(viewId, done) {
		restoreViewInput.m_id = "" + viewId
		restoreViewRequestSender.callback = function(ok){
			if (ok){
				root.viewCreated(viewId)
			}
			done(ok)
		}
		restoreViewRequestSender.send(restoreViewInput)
	}

	// The service name is owned by the agent: the service is renamed the same way the service
	// editor saves it (GetService, then UpdateService with the new name, forwarded to the agent).
	// agentId: owning agent (clientid header); empty in the agent app.
	function opRenameService(serviceId, agentId, name, done) {
		let controller = serviceRenameControllerComp.createObject(root)
		if (!controller){
			root.groupOperationFailed(qsTr("Unable to rename the service"))
			done(false)
			return
		}
		controller.callback = done
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
			property var callback: null

			function complete(ok) {
				let done = renameController.callback
				renameController.callback = null
				if (done){
					done(ok)
				}
				// Destroyed after the request finished, not from inside its handler.
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
				renameController.complete(true)
			}

			// The base controller already shows the error message.
			onError: {
				renameController.complete(false)
			}
		}
	}

	// --- Topology views (ways of grouping) ---

	// SDL list of strings (JS array or list-like object) as a JS array; null gives an empty array.
	function toStringList(value) {
		let result = []
		if (value === undefined || value === null){
			return result
		}
		let count = value.length !== undefined ? value.length : 0
		for (let i = 0; i < count; ++i){
			if (value[i] !== undefined && value[i] !== null){
				result.push("" + value[i])
			}
		}
		return result
	}

	function loadViews() {
		if (root.viewsUnsupported){
			return
		}
		getViewsRequestSender.send()
	}

	property CreateTopologyViewInput createViewInput: CreateTopologyViewInput {}
	property RemoveTopologyViewInput removeViewInput: RemoveTopologyViewInput {}
	property RestoreTopologyViewInput restoreViewInput: RestoreTopologyViewInput {}

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
								views.push({"id": item.m_id, "name": item.m_name, "nodeNames": root.toStringList(item.m_nodeNames)})
							}
						}
					}
					root.viewsSupported = true
					root.viewsLoaded(views, root.toStringList(m_agentsViewNodeNames))
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

	// The senders of the changes: the response completes the operation (callback), an error shows
	// a message and fails it; "finished(-1)" also covers transport errors.
	property GqlSdlRequestSender createViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "CreateTopologyView"
		sdlObjectComp: Component {
			CreateTopologyViewResponse {
				onFinished: root.completeOperation(root.createViewRequestSender, true, m_id)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.createViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.createViewRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RemoveTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.removeViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.removeViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.removeViewRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender restoreViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RestoreTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.restoreViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.restoreViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.restoreViewRequestSender, false)
			finished(-1)
		}
	}

	property CreateTopologyGroupInput createGroupInput: CreateTopologyGroupInput {}
	property MoveTopologyNodesInput moveNodesInput: MoveTopologyNodesInput {}
	property RemoveTopologyGroupInput removeGroupInput: RemoveTopologyGroupInput {}
	property RenameTopologyGroupInput renameGroupInput: RenameTopologyGroupInput {}
	property RemoveFromTopologyViewInput removeFromViewInput: RemoveFromTopologyViewInput {}
	property AddToTopologyViewInput addToViewInput: AddToTopologyViewInput {}

	property GqlSdlRequestSender addToViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "AddToTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.addToViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.addToViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.addToViewRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeFromViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RemoveFromTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.removeFromViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.removeFromViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.removeFromViewRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender renameGroupRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RenameTopologyGroup"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.renameGroupRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.renameGroupRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.renameGroupRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender createGroupRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "CreateTopologyGroup"
		sdlObjectComp: Component {
			CreateTopologyGroupResponse {
				onFinished: root.completeOperation(root.createGroupRequestSender, true, m_id)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.createGroupRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.createGroupRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender moveNodesRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "MoveTopologyNodes"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.moveNodesRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.moveNodesRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.moveNodesRequestSender, false)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeGroupRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RemoveTopologyGroup"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeOperation(root.removeGroupRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeOperation(root.removeGroupRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			root.completeOperation(root.removeGroupRequestSender, false)
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
			// The views are found by the names of their services as well.
			root.loadViews()
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
		property var callback: null
		requestType: 1
		// Wire names (same as AgentinoTopologySdlCommandIds) — string literals so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "SaveTopology"
		sdlObjectComp: Component {
			SaveTopologyResponse {
				onFinished: root.completeOperation(root.saveModelRequestSender, m_successful !== false)
			}
		}
		// status -1 = error; the callback clears the save gate so later OnTopologyChanged works.
		onFinished: {
			if (status < 0){
				console.error("SaveTopology failed")
				root.completeOperation(root.saveModelRequestSender, false)
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
