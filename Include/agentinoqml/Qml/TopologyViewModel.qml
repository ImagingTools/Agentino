import QtQuick 2.12
import Acf 1.0
import imtguigql 1.0
import agentino 1.0
import agentinoTopologySdl 1.0
import agentinoServicesSdl 1.0

/**
	L2/L3 Topology port + GQL adapter (Architecture §6.4 / QG2).
	View binds only to properties/signals — no gqlCommandId in the view file.

	The view (way of grouping) is edited as a local draft: the changes (positions, groups, membership,
	services of a user-defined view) are applied to currentState only; Undo/Redo switch between the
	states of the draft; save() sends the whole state of the view to the server with one SaveTopology.
*/
QtObject {
	id: root

	property var dataScope: DataScope {}
	property var serviceStatusModel: ServiceStatusModel {}
	property var objectsModel: null
	property bool loading: false
	property string errorMessage: ""
	// Current view (way of grouping); empty for the default view grouping services by agents.
	// Use setView() to change it.
	property string viewId: ""
	// False until GetTopologyViews succeeds (e.g. the agent app has no views).
	property bool viewsSupported: false
	// Set when the server answered that views are not supported; transport failures do not set it.
	property bool viewsUnsupported: false

	// Every change is saved at once (the agent app has no Save command).
	property bool autoSave: false

	signal viewStateChanged()
	signal topologySaved()
	signal serviceStatusMessage(var data)
	signal collectionChanged()
	signal agentStatusChanged()
	signal groupOperationFailed(string message)
	// views: [{id, name, nodeNames}] of the user-defined views (the default view is not listed);
	// nodeNames: names of the services and groups of the view, agentsViewNodeNames - the same for the default view.
	signal viewsLoaded(var views, var agentsViewNodeNames)
	signal viewCreated(string viewId)
	signal viewRemoved(string viewId)
	// A removed view was restored (Undo of the removal, Redo of the creation).
	signal viewRestored(string viewId)

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

	// --- State of the view ---
	// A state is {viewId, nodes: {id: {id, nodeType, parentId, agentId, x, y, name}}} with all nodes shown
	// in the view: services, agents (default view) and groups. parentId is the containing agent or group
	// ("" for the root level); the members of a group are the nodes with its ID as parentId.

	// The state as loaded from the server; null until the view is loaded.
	property var baseState: null
	// The draft: the state shown and edited on the client.
	property var currentState: null
	// The draft differs from the server state.
	property bool dirty: false
	property bool saving: false
	readonly property bool canSave: root.dirty && !root.saving

	// Nodes of the last loaded topology by ID (SDL Service objects shown on the scheme): the nodes of the
	// view and the services that can be added to it (user-defined view), plus groups created in the draft.
	property var nodeItems: ({})
	// Names of all known services by ID ("service@agent").
	property var serviceNames: ({})
	// Services that are not in the draft of the user-defined view and can be added to it: [{id, name}].
	property var availableServices: []

	// History of Undo/Redo. A step is either
	//   {type: "state", state} - a state of the draft of the current view (the steps of other views are
	//                            dropped when another view is shown: their draft was saved or discarded), or
	//   {type: "view", action: "create"/"remove", viewId} - a view was created / removed. Views are not a
	//                            part of a draft: these steps are applied on the server at once
	//                            (RemoveTopologyView / RestoreTopologyView).
	readonly property int maxHistorySteps: 50
	property var undoStack: []
	property var redoStack: []
	property int undoCount: 0
	property int redoCount: 0
	// A view step is applied on the server: it has left one stack and is not in the other one yet.
	property bool viewOperationPending: false
	readonly property bool canUndo: root.undoCount > 0 && !root.viewOperationPending
	readonly property bool canRedo: root.redoCount > 0 && !root.viewOperationPending

	function copyNode(node) {
		return {
			"id": node.id,
			"nodeType": node.nodeType,
			"parentId": node.parentId,
			"agentId": node.agentId,
			"x": node.x,
			"y": node.y,
			"name": node.name
		}
	}

	function copyState(state) {
		let nodes = {}
		for (let id in state.nodes){
			nodes[id] = root.copyNode(state.nodes[id])
		}
		return {"viewId": state.viewId, "nodes": nodes}
	}

	function nodesEqual(left, right) {
		return left.parentId === right.parentId && left.x === right.x && left.y === right.y && left.name === right.name
	}

	function statesEqual(left, right) {
		if (!left || !right){
			return left === right
		}
		if (left.viewId !== right.viewId){
			return false
		}
		let count = 0
		for (let id in left.nodes){
			let other = right.nodes[id]
			if (!other || !root.nodesEqual(left.nodes[id], other)){
				return false
			}
			++count
		}
		return count === Object.keys(right.nodes).length
	}

	// Place of a node outside of all groups: in the default view a service belongs to its agent,
	// otherwise the node is at the root level.
	function defaultParentId(state, node) {
		if (state.viewId === "" && node.nodeType === "SERVICE" && node.agentId && state.nodes[node.agentId]){
			return node.agentId
		}
		return ""
	}

	// Parents that are not in the state anymore are replaced by the default place; groups nested into
	// themselves (e.g. after a merge with changes of another client) are moved out of the cycle.
	function normalizeState(state) {
		for (let id in state.nodes){
			let node = state.nodes[id]
			let parent = node.parentId !== "" ? state.nodes[node.parentId] : null
			if (node.parentId !== "" && (!parent || (parent.nodeType !== "GROUP" && parent.nodeType !== "AGENT"))){
				node.parentId = root.defaultParentId(state, node)
			}
		}
		for (let id in state.nodes){
			let visitedIds = {}
			let currentId = state.nodes[id].parentId
			while (currentId !== "" && state.nodes[currentId] && !visitedIds[currentId]){
				if (currentId === id){
					state.nodes[id].parentId = root.defaultParentId(state, state.nodes[id])
					break
				}
				visitedIds[currentId] = true
				currentId = state.nodes[currentId].parentId
			}
		}
		return state
	}

	// Applies the changes of the server (base -> server, e.g. made by another client) to the draft: nodes
	// added on the server are added, nodes removed there are removed; a property of a node takes the value
	// of the server unless it was changed in the draft. The draft keeps its own changes.
	function mergeState(base, server, draft) {
		let result = {"viewId": draft.viewId, "nodes": {}}
		for (let id in draft.nodes){
			let draftNode = draft.nodes[id]
			let baseNode = base.nodes[id]
			let serverNode = server.nodes[id]
			if (baseNode && !serverNode){
				continue
			}
			if (!serverNode){
				// Added in the draft: a new group, or a service added to the user-defined view that still exists.
				if (draftNode.nodeType === "GROUP" || root.serviceNames[id] !== undefined){
					result.nodes[id] = root.copyNode(draftNode)
				}
				continue
			}
			let node = root.copyNode(draftNode)
			if (baseNode){
				let keys = ["parentId", "x", "y", "name"]
				for (let i = 0; i < keys.length; ++i){
					if (draftNode[keys[i]] === baseNode[keys[i]]){
						node[keys[i]] = serverNode[keys[i]]
					}
				}
			}
			node.agentId = serverNode.agentId
			result.nodes[id] = node
		}
		for (let id in server.nodes){
			if (!base.nodes[id] && !draft.nodes[id]){
				result.nodes[id] = root.copyNode(server.nodes[id])
			}
		}
		return root.normalizeState(result)
	}

	function updateHistoryCounts() {
		root.undoCount = root.undoStack.length
		root.redoCount = root.redoStack.length
	}

	function stateStep(state) {
		return {"type": "state", "state": state}
	}

	// Records a new step: the undone steps cannot be redone anymore.
	function pushHistoryStep(step) {
		let stack = root.undoStack.slice()
		stack.push(step)
		while (stack.length > root.maxHistorySteps){
			stack.shift()
		}
		root.undoStack = stack
		root.redoStack = []
		root.updateHistoryCounts()
	}

	// Drops the steps of the draft (it was saved or discarded, or another view is shown); the steps of
	// the views stay.
	function clearDraftHistory() {
		let isViewStep = function(step){ return step.type === "view" }
		root.undoStack = root.undoStack.filter(isViewStep)
		root.redoStack = root.redoStack.filter(isViewStep)
		root.updateHistoryCounts()
	}

	function updateAvailableServices() {
		let result = []
		if (root.currentState && root.currentState.viewId !== ""){
			for (let id in root.serviceNames){
				if (!root.currentState.nodes[id]){
					result.push({"id": id, "name": root.serviceNames[id]})
				}
			}
		}
		result.sort(function(left, right){ return left.name.localeCompare(right.name) })
		root.availableServices = result
	}

	function updateState() {
		root.dirty = root.currentState !== null && !root.statesEqual(root.currentState, root.baseState)
		root.updateHistoryCounts()
		root.updateAvailableServices()
		root.viewStateChanged()
	}

	// Makes newState the current state of the draft as one step of the history.
	function commit(newState) {
		if (!root.currentState){
			return
		}
		root.normalizeState(newState)
		if (root.statesEqual(newState, root.currentState)){
			return
		}
		root.pushHistoryStep(root.stateStep(root.currentState))
		root.currentState = newState
		root.updateState()
		root.saveIfAutoSave()
	}

	// change: function(state) changing a copy of the current state.
	function edit(change) {
		if (!root.currentState){
			return
		}
		let state = root.copyState(root.currentState)
		change(state)
		root.commit(state)
	}

	function undo() {
		root.stepHistory(true)
	}

	function redo() {
		root.stepHistory(false)
	}

	// Undoes (isUndo) or redoes the last step.
	function stepHistory(isUndo) {
		if (isUndo ? !root.canUndo : !root.canRedo){
			return
		}
		let fromStack = (isUndo ? root.undoStack : root.redoStack).slice()
		let step = fromStack[fromStack.length - 1]

		if (step.type === "view"){
			fromStack.pop()
			if (isUndo){
				root.undoStack = fromStack
			}
			else{
				root.redoStack = fromStack
			}
			root.updateHistoryCounts()
			root.applyViewStep(step, isUndo)
			return
		}

		if (!root.currentState){
			return
		}
		fromStack.pop()
		let toStack = (isUndo ? root.redoStack : root.undoStack).slice()
		toStack.push(root.stateStep(root.currentState))
		root.undoStack = isUndo ? fromStack : toStack
		root.redoStack = isUndo ? toStack : fromStack
		root.currentState = step.state
		root.updateState()
		root.saveIfAutoSave()
	}

	// Applies a view step on the server: undo of a creation and redo of a removal remove the view,
	// the others restore it. The step goes to the other stack if it succeeded, otherwise it stays.
	function applyViewStep(step, isUndo) {
		let remove = (step.action === "create") === isUndo
		root.viewOperationPending = true
		let done = function(ok){
			root.viewOperationPending = false
			// The stacks may have been changed meanwhile (e.g. the steps of the draft dropped on a view change).
			let stack = ((ok === isUndo) ? root.redoStack : root.undoStack).slice()
			stack.push(step)
			if (ok === isUndo){
				root.redoStack = stack
			}
			else{
				root.undoStack = stack
			}
			root.updateHistoryCounts()
		}
		if (remove){
			root.opRemoveView(step.viewId, done)
		}
		else{
			root.opRestoreView(step.viewId, done)
		}
	}

	// Reverts the draft to the server state.
	function discardChanges() {
		if (!root.baseState){
			return
		}
		root.currentState = root.copyState(root.baseState)
		root.clearDraftHistory()
		root.updateState()
	}

	// Shows another view; the draft of the current view is dropped (see TopologyPage.selectView).
	function setView(viewId) {
		root.viewId = viewId
		root.baseState = null
		root.currentState = null
		root.clearDraftHistory()
		root.updateState()
		root.loadTopology()
	}

	// --- Changes of the draft (one step of the history each) ---

	// positions: [{id, x, y}] of the moved nodes.
	function setPositions(positions) {
		root.edit(function(state){
			for (let i = 0; i < positions.length; ++i){
				let node = state.nodes[positions[i].id]
				if (node){
					node.x = positions[i].x
					node.y = positions[i].y
				}
			}
		})
	}

	// IDs of new groups are generated on the client; the server creates the groups with them on save.
	function createGroupId() {
		return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function(c){
			let r = Math.floor(Math.random() * 16)
			return (c === "x" ? r : (r & 0x3 | 0x8)).toString(16)
		})
	}

	function addGroup(state, name, parentId, x, y) {
		let groupId = root.createGroupId()
		while (state.nodes[groupId] || root.nodeItems[groupId]){
			groupId = root.createGroupId()
		}
		state.nodes[groupId] = {
			"id": groupId,
			"nodeType": "GROUP",
			"parentId": parentId ? "" + parentId : "",
			"agentId": "",
			"x": x,
			"y": y,
			"name": "" + name
		}
		return groupId
	}

	// parentId: group containing the new group, empty for the root level.
	function createGroup(name, parentId, x, y) {
		root.edit(function(state){
			root.addGroup(state, name, parentId, x, y)
		})
	}

	// parentId: target group; empty returns the nodes to their default place
	// (in the default view a service to its agent, otherwise the root level).
	function moveNodesInState(state, nodeIds, parentId) {
		for (let i = 0; i < nodeIds.length; ++i){
			let node = state.nodes[nodeIds[i]]
			if (node){
				node.parentId = parentId ? "" + parentId : root.defaultParentId(state, node)
			}
		}
	}

	function moveNodes(nodeIds, parentId) {
		root.edit(function(state){
			root.moveNodesInState(state, nodeIds, parentId)
		})
	}

	// Creates a group and moves the nodes into it ("Add to group" when there is no group to add them to).
	function createGroupWithNodes(name, parentId, x, y, nodeIds) {
		root.edit(function(state){
			let groupId = root.addGroup(state, name, parentId, x, y)
			root.moveNodesInState(state, nodeIds, groupId)
		})
	}

	// Removes the groups; their members are moved one level up (members of a root group go to their
	// default place) - the same as RemoveTopologyGroup of the server. Used by Remove and Ungroup.
	function removeGroups(groupIds) {
		root.edit(function(state){
			for (let i = 0; i < groupIds.length; ++i){
				let group = state.nodes[groupIds[i]]
				if (!group || group.nodeType !== "GROUP"){
					continue
				}
				for (let id in state.nodes){
					let node = state.nodes[id]
					if (node.parentId === group.id){
						node.parentId = group.parentId !== "" ? group.parentId : root.defaultParentId(state, node)
					}
				}
				delete state.nodes[group.id]
			}
		})
	}

	function removeGroup(groupId) {
		root.removeGroups([groupId])
	}

	function renameGroup(groupId, name) {
		root.edit(function(state){
			let group = state.nodes[groupId]
			if (group && group.nodeType === "GROUP"){
				group.name = "" + name
			}
		})
	}

	// Removes the services from the current user-defined view only; the services are not deleted.
	function removeFromView(nodeIds) {
		if (root.viewId === ""){
			return
		}
		root.edit(function(state){
			for (let i = 0; i < nodeIds.length; ++i){
				let node = state.nodes[nodeIds[i]]
				if (node && node.nodeType === "SERVICE"){
					delete state.nodes[node.id]
				}
			}
		})
	}

	// Adds services (not yet in the current user-defined view) to it.
	// parentId: group of the view receiving the services; empty for the root level.
	function addToView(nodeIds, parentId) {
		if (root.viewId === ""){
			return
		}
		root.edit(function(state){
			for (let i = 0; i < nodeIds.length; ++i){
				let id = "" + nodeIds[i]
				if (state.nodes[id] || root.serviceNames[id] === undefined){
					continue
				}
				let item = root.nodeItems[id]
				state.nodes[id] = {
					"id": id,
					"nodeType": "SERVICE",
					"parentId": parentId ? "" + parentId : "",
					"agentId": item ? "" + item.m_agentId : "",
					// The position the service had in the view, if any; otherwise a free place is found.
					"x": item ? item.m_x : 0,
					"y": item ? item.m_y : 0,
					"name": root.serviceNames[id]
				}
			}
		})
	}

	// --- Nodes shown on the scheme ---

	property Component topologyComp: Component { Topology {} }
	property var nodeFactory: null

	// SDL object of the node shown on the scheme. Nodes created in the draft get an object of their own.
	function getNodeItem(nodeId) {
		let item = root.nodeItems[nodeId]
		if (item){
			return item
		}
		let node = root.currentState ? root.currentState.nodes[nodeId] : null
		if (!node){
			return null
		}
		if (!root.nodeFactory){
			root.nodeFactory = root.topologyComp.createObject(root)
		}
		// With a parent: in JQML3 an object without a parent is destroyed once no model holds it, but the
		// node is shown again after Undo or when its level is opened again.
		item = root.nodeFactory.createElement('m_services').createObject(root.nodeFactory)
		if (!item){
			return null
		}
		let isGroup = node.nodeType === "GROUP"
		item.m_id = node.id
		item.m_agentId = node.agentId
		item.m_nodeType = node.nodeType
		item.m_isComposite = isGroup
		item.m_mainText = node.name
		item.m_thirdText = isGroup ? "Group" : ""
		item.m_agentOnline = true
		item.m_status = "UNDEFINED"
		item.m_icon1 = isGroup ? "" : "Icons/Alert"
		item.m_parentId = node.parentId
		item.m_x = node.x
		item.m_y = node.y
		item.emplaceLinks()

		let items = root.nodeItems
		items[nodeId] = item
		root.nodeItems = items
		return item
	}

	// --- Loading ---

	function loadTopology() {
		if (!getTopologyRequestSender.gqlCommandId){
			console.error("GetTopology: gqlCommandId is empty")
			root.loading = false
			return
		}
		// The server state is changing while the draft is saved: loaded after the save.
		if (root.saving){
			root.reloadPending = true
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

	// Reads the loaded topology: the state of the server and the nodes shown on the scheme. The draft is
	// merged with the changes of the server.
	function applyServerTopology(topologySdlObject, viewId) {
		let serverState = {"viewId": viewId, "nodes": {}}
		let items = {}
		let names = {}

		let readNodes = function(nodesModel, isInView){
			if (!nodesModel){
				return
			}
			for (let i = 0; i < nodesModel.count; ++i){
				let item = nodesModel.get(i).item
				if (!item){
					continue
				}
				let id = "" + item.m_id
				let nodeType = item.m_nodeType ? "" + item.m_nodeType : "SERVICE"
				items[id] = item
				if (nodeType === "SERVICE"){
					names[id] = "" + item.m_mainText
				}
				if (isInView){
					serverState.nodes[id] = {
						"id": id,
						"nodeType": nodeType,
						"parentId": item.m_parentId ? "" + item.m_parentId : "",
						"agentId": item.m_agentId ? "" + item.m_agentId : "",
						"x": item.m_x,
						"y": item.m_y,
						"name": "" + item.m_mainText
					}
				}
			}
		}
		readNodes(topologySdlObject.m_services, true)
		readNodes(topologySdlObject.m_availableServiceNodes, false)

		// Services that can be added to the view (their names, also if the nodes are not sent).
		let availableModel = topologySdlObject.m_availableServices
		if (availableModel){
			for (let i = 0; i < availableModel.count; ++i){
				let info = availableModel.get(i).item
				if (info && names["" + info.m_id] === undefined){
					names["" + info.m_id] = info.m_name ? "" + info.m_name : "" + info.m_id
				}
			}
		}

		// Nodes created in the draft keep their objects.
		if (root.currentState){
			for (let id in root.nodeItems){
				if (!items[id] && root.currentState.nodes[id] && root.currentState.nodes[id].nodeType === "GROUP"){
					items[id] = root.nodeItems[id]
				}
			}
		}

		root.nodeItems = items
		root.serviceNames = names
		root.normalizeState(serverState)

		if (!root.baseState || !root.currentState || root.baseState.viewId !== viewId){
			root.baseState = serverState
			root.currentState = root.copyState(serverState)
			root.clearDraftHistory()
		}
		else if (!root.statesEqual(root.baseState, serverState)){
			let base = root.baseState
			let mergeStep = function(step){
				return step.type === "state" ? root.stateStep(root.mergeState(base, serverState, step.state)) : step
			}
			root.currentState = root.mergeState(base, serverState, root.currentState)
			root.undoStack = root.undoStack.map(mergeStep)
			root.redoStack = root.redoStack.map(mergeStep)
			root.baseState = serverState
		}
		root.updateState()
	}

	// --- Saving ---

	// The whole state of the view: positions of all nodes, all groups with their members and, in a
	// user-defined view, the services shown in it.
	function buildTopologyInput(state) {
		let topologyInput = topologyInputComp.createObject(root)
		topologyInput.m_viewId = state.viewId

		// SDL-generated lists are QVariants that only accept the generated list objects (built via
		// emplace...()/create...ArrayElement()/addElement()); plain JS values are silently rejected.
		topologyInput.emplaceServices()
		topologyInput.emplaceGroups()
		let serviceIds = []
		for (let id in state.nodes){
			let node = state.nodes[id]
			let coordinate = topologyInput.createServicesArrayElement()
			coordinate.m_id = node.id
			coordinate.m_x = node.x
			coordinate.m_y = node.y
			topologyInput.m_services.addElement(coordinate)

			if (node.nodeType === "GROUP"){
				let memberIds = []
				for (let memberId in state.nodes){
					if (state.nodes[memberId].parentId === node.id){
						memberIds.push(memberId)
					}
				}
				let group = topologyInput.createGroupsArrayElement()
				group.m_id = node.id
				group.m_name = node.name
				group.m_x = node.x
				group.m_y = node.y
				group.m_memberIds = memberIds
				topologyInput.m_groups.addElement(group)
			}
			else if (node.nodeType === "SERVICE"){
				serviceIds.push(node.id)
			}
		}
		if (state.viewId !== ""){
			topologyInput.m_nodeIds = serviceIds
		}
		return topologyInput
	}

	property Component topologyInputComp: Component { SaveTopologyInput {} }

	// Sends the whole state of the view to the server. done(ok) is called once the save has finished
	// (also with true if there was nothing to save).
	function save(done) {
		if (root.saving){
			if (done){
				done(false)
			}
			return
		}
		if (!root.dirty || !root.currentState){
			if (done){
				done(true)
			}
			return
		}
		if (!saveModelRequestSender.gqlCommandId){
			console.error("SaveTopology: gqlCommandId is empty")
			if (done){
				done(false)
			}
			return
		}

		let sentState = root.copyState(root.currentState)
		let topologyInput = root.buildTopologyInput(sentState)
		root.saving = true
		saveModelRequestSender.callback = function(ok){
			root.saving = false
			// The saved state is the new server state of the view (the draft may have been changed meanwhile).
			if (ok && root.baseState && root.baseState.viewId === sentState.viewId){
				root.baseState = sentState
				root.updateState()
				root.topologySaved()
			}
			// Group names are shown in the views panel; a failed save may have changed a part of the view.
			root.loadViews()
			root.requestReload()
			if (done){
				done(ok)
			}
			// Changes made during the save (auto save).
			root.saveIfAutoSave()
		}
		saveModelRequestSender.send(topologyInput)
		topologyInput.destroy()
	}

	function saveIfAutoSave() {
		if (root.autoSave && root.dirty && !root.saving){
			root.save()
		}
	}

	onAutoSaveChanged: root.saveIfAutoSave()

	// --- Changes applied on the server at once (not a part of the view state) ---

	// The service name is owned by the agent (see opRenameService).
	function renameService(serviceId, agentId, name) {
		root.opRenameService("" + serviceId, agentId ? "" + agentId : "", "" + name, function(ok){
			root.loadViews()
			root.requestReload()
		})
	}

	// Creates a view on the server at once; recorded in the history (undo removes it, redo restores it).
	function createView(name) {
		createViewInput.m_name = "" + name
		createViewRequestSender.callback = function(ok, viewId){
			if (ok){
				root.pushHistoryStep({"type": "view", "action": "create", "viewId": viewId})
				root.loadViews()
				root.viewCreated(viewId)
			}
		}
		createViewRequestSender.send(createViewInput)
	}

	// Removes a view on the server at once (it is kept marked as removed); recorded in the history
	// (undo restores it, redo removes it again).
	function removeView(viewId) {
		let id = "" + viewId
		root.opRemoveView(id, function(ok){
			if (ok){
				root.pushHistoryStep({"type": "view", "action": "remove", "viewId": id})
			}
		})
	}

	// done(ok) is called once. A removed view is not shown anymore (viewRemoved).
	function opRemoveView(viewId, done) {
		removeViewInput.m_id = "" + viewId
		removeViewRequestSender.callback = function(ok){
			root.loadViews()
			if (ok){
				root.viewRemoved("" + viewId)
			}
			done(ok)
		}
		removeViewRequestSender.send(removeViewInput)
	}

	// done(ok) is called once. The restored view is shown (viewRestored).
	function opRestoreView(viewId, done) {
		restoreViewInput.m_id = "" + viewId
		restoreViewRequestSender.callback = function(ok){
			root.loadViews()
			if (ok){
				root.viewRestored("" + viewId)
			}
			done(ok)
		}
		restoreViewRequestSender.send(restoreViewInput)
	}

	// Calls the callback of the request once.
	function completeRequest(sender, ok, result) {
		let callback = sender.callback
		sender.callback = null
		if (callback){
			callback(ok, result)
		}
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

	property RestoreTopologyViewInput restoreViewInput: RestoreTopologyViewInput {}

	// The senders of the view changes: the response completes the request (callback), an error shows
	// a message and fails it; "finished(-1)" also covers transport errors.
	property GqlSdlRequestSender createViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "CreateTopologyView"
		sdlObjectComp: Component {
			CreateTopologyViewResponse {
				onFinished: root.completeRequest(root.createViewRequestSender, true, "" + m_id)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeRequest(root.createViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender removeViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RemoveTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeRequest(root.removeViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeRequest(root.removeViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	property GqlSdlRequestSender restoreViewRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		gqlCommandId: "RestoreTopologyView"
		sdlObjectComp: Component {
			TopologyGroupResponse {
				onFinished: root.completeRequest(root.restoreViewRequestSender, m_successful !== false)
			}
		}
		onFinished: {
			if (status < 0){
				root.completeRequest(root.restoreViewRequestSender, false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
		}
	}

	function requestReload() {
		if (root.saving){
			root.reloadPending = true
			return
		}
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

	function completeSave(ok) {
		let callback = saveModelRequestSender.callback
		saveModelRequestSender.callback = null
		if (callback){
			callback(ok)
		}
	}

	property GqlSdlRequestSender saveModelRequestSender: GqlSdlRequestSender {
		property var callback: null
		requestType: 1
		// Wire names (same as AgentinoTopologySdlCommandIds) — string literals so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "SaveTopology"
		sdlObjectComp: Component {
			SaveTopologyResponse {
				onFinished: root.completeSave(m_successful !== false)
			}
		}
		// status -1 = error (also transport errors); the callback clears the save gate.
		onFinished: {
			if (status < 0){
				console.error("SaveTopology failed")
				root.completeSave(false)
			}
		}
		function onError(message, type) {
			root.groupOperationFailed(message)
			finished(-1)
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
					if (root.reloadPending || root.saving || root.getTopologyRequestSender.requestedViewId !== root.viewId){
						root.loadTopology()
						return
					}
					root.applyServerTopology(this, root.getTopologyRequestSender.requestedViewId)
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
