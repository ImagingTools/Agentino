import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0
import agentino 1.0
import agentinoServicesSdl 1.0

ViewBase {
	id: topologyPage;
	
	anchors.fill: parent;
	clip: true;
	
	property var documentManager: MainDocumentService.getDocumentService("Topology");
	property string collectionId: "Services"
	property string selectedAgentId: ""
	property bool serviceOperationInProgress: false
	property string serviceOperationText: ""

	// Navigation through composite nodes (agents and groups): every level of schemeStack is a scheme
	// showing the direct children of its node; the first level is the whole topology (groupId "").
	// Scheme of the current level; null until the first level is loaded.
	property var scheme: null
	// Composite node shown by the current level.
	readonly property string groupId: scheme ? scheme.groupId : ""
	readonly property string selectedService: scheme ? scheme.selectedService : ""
	// Node IDs of the levels of schemeStack (and of the headers of the navigation panel).
	property var levelNodeIds: [""]
	// Agent of the service document being opened or created (clientid of the document components).
	property string documentAgentId: ""
	// Nodes of the last loaded topology by ID: {id, parentId, nodeType, name, agentId}.
	property var topologyNodes: ({})
	// Set while the selection is restored after a reload (the selection is applied once afterwards).
	property bool reselecting: false

	// Views (ways of grouping the nodes): the pinned default view "Agents" (empty ID) groups services
	// by agents, the user-defined views group them freely. Items: {id, name, pinned}.
	property ListModel viewsModel: ListModel {}
	readonly property string viewId: topologyViewModel.viewId
	// Services that are not in the current user-defined view and can be added to it: [{id, name}].
	property var availableServices: []

	// Transport stays in TopologyViewModel (L2/L3) — view is presentation only (QG2).
	property var topologyViewModel: TopologyViewModel {}
	
	//for scrollBars
	property real originX: 0;
	property real originY: 0;
	//for scrollBars
	
	function setServiceOperation(inProgress, description){
		serviceOperationInProgress = inProgress
		serviceOperationText = description
	}

	// Topology.Service.status is SDL enum wire format (RUNNING / NOT_RUNNING / ...).
	function normalizeServiceStatus(status){
		return topologyViewModel.normalizeServiceStatus(status)
	}

	// Meta-info only for a service that is still on the canvas — avoids GetElementMetaInfo
	// errors after Remove when selection/subscription still holds a deleted id.
	function refreshSelectedMetaInfo(){
		if (topologyPage.selectedService === "")
			return
		if (topologyPage.scheme.findModelIndex(topologyPage.selectedService) < 0)
			return
		collectionDataProvider.getObjectMetaInfo(topologyPage.selectedService)
	}

	function clearServiceSelection(){
		if (topologyPage.scheme){
			topologyPage.scheme.selectedIndex = -1
			topologyPage.scheme.selectedNodeId = ""
			topologyPage.scheme.selectedService = ""
			topologyPage.scheme.setSelectedLink(null)
		}
		topologyPage.selectedAgentId = ""
		metaInfo.contentVisible = false
		topologyPage.updateCommandStates()
	}

	// Services linked through the clicked link of the shown level (see updateLinkInfo); null if no link is selected.
	// {fromName, toName, pairs: [{source: {id, name, path, agentId}, target: {...}}]}
	property var linkInfo: null

	function updateLinkInfo(){
		let currentScheme = topologyPage.scheme
		let link = currentScheme ? currentScheme.selectedLink : null
		if (!link){
			topologyPage.linkInfo = null
			return
		}
		let info = topologyPage.computeLinkInfo(link.fromId, link.toId, currentScheme.groupId)
		topologyPage.linkInfo = info
		// The link is no longer on the level (e.g. a service was moved by another client).
		if (info === null){
			currentScheme.setSelectedLink(null)
		}
	}

	function makeServiceInfo(serviceId){
		let node = topologyPage.topologyNodes[serviceId]
		return {
			"id": serviceId,
			"name": node ? "" + node.name : "" + serviceId,
			"path": node ? topologyPage.getNodePath(node.parentId) : "",
			"agentId": node && node.agentId ? "" + node.agentId : ""
		}
	}

	// Pairs of services (the service and the service it is linked with) whose link is drawn between the
	// shown nodes firstId and secondId of the level groupId: the nodes themselves or groups / agents
	// containing the services. null if there are none.
	function computeLinkInfo(firstId, secondId, groupId){
		let pairs = []
		for (let sourceId in topologyPage.topologyNodes){
			let linkIds = topologyPage.topologyNodes[sourceId].linkIds
			if (!linkIds || linkIds.length === 0)
				continue
			let shownSourceId = topologyPage.findShownNode(sourceId, groupId)
			if (shownSourceId !== firstId && shownSourceId !== secondId)
				continue
			for (let k = 0; k < linkIds.length; ++k){
				let targetId = linkIds[k]
				let shownTargetId = topologyPage.findShownNode(targetId, groupId)
				if (shownTargetId === shownSourceId || (shownTargetId !== firstId && shownTargetId !== secondId))
					continue
				pairs.push({"source": topologyPage.makeServiceInfo(sourceId), "target": topologyPage.makeServiceInfo(targetId)})
			}
		}
		if (pairs.length === 0)
			return null

		pairs.sort(function(left, right){
			let result = left.source.name.localeCompare(right.source.name)
			return result !== 0 ? result : left.target.name.localeCompare(right.target.name)
		})
		let firstNode = topologyPage.topologyNodes[firstId]
		let secondNode = topologyPage.topologyNodes[secondId]
		return {
			"fromName": firstNode ? "" + firstNode.name : "",
			"toName": secondNode ? "" + secondNode.name : "",
			"pairs": pairs
		}
	}

	// The selected node when exactly one node is selected, null otherwise (none or several).
	function selectedNode(){
		let currentScheme = topologyPage.scheme
		if (!currentScheme || currentScheme.selectedIndex < 0 || !currentScheme.objectsModel
				|| currentScheme.selectedIndex >= currentScheme.objectsModel.count
				|| currentScheme.selectedIds.length > 1)
			return null
		let item = currentScheme.objectsModel.get(currentScheme.selectedIndex).item
		return item ? item : null
	}

	// All selected nodes of the shown level (Ctrl+click / selection rectangle select several).
	function selectedNodes(){
		let result = []
		let currentScheme = topologyPage.scheme
		if (!currentScheme || !currentScheme.objectsModel)
			return result
		let indexes = currentScheme.selectedObjectIndexes()
		for (let i = 0; i < indexes.length; ++i){
			let item = currentScheme.objectsModel.get(indexes[i]).item
			if (item){
				result.push(item)
			}
		}
		return result
	}

	function selectedNodeIds(){
		let items = topologyPage.selectedNodes()
		let result = []
		for (let i = 0; i < items.length; ++i){
			result.push(items[i].m_id)
		}
		return result
	}

	// Ungroup is applied to user-defined groups only (not to agents) that contain anything: ungrouping
	// an empty group would only remove it. Empty if any selected node is not such a group.
	function selectedGroupIds(){
		let items = topologyPage.selectedNodes()
		let result = []
		for (let i = 0; i < items.length; ++i){
			if (!topologyPage.isGroupNode(items[i].m_id) || !topologyPage.hasMembers(items[i].m_id))
				return []
			result.push(items[i].m_id)
		}
		return result
	}

	// True if the composite node (group or agent) contains any node in the draft.
	function hasMembers(nodeId){
		for (let id in topologyPage.topologyNodes){
			if (topologyPage.topologyNodes[id].parentId === nodeId)
				return true
		}
		return false
	}

	// Agents and groups are composite nodes; double click opens their content as the next level.
	function isCompositeNode(item){
		return item !== null && item !== undefined && item.m_isComposite === true
	}

	// User-defined group (an agent is composite as well, but its content is fixed).
	function isGroupNode(nodeId){
		let node = topologyPage.topologyNodes[nodeId]
		return node !== undefined && node.nodeType === "GROUP"
	}

	// True if the node is the ancestor node itself or is nested in it.
	function isNodeInside(nodeId, ancestorId){
		let visitedIds = {}
		let currentId = nodeId
		while (currentId && !visitedIds[currentId]){
			if (currentId === ancestorId)
				return true
			visitedIds[currentId] = true
			let node = topologyPage.topologyNodes[currentId]
			currentId = node ? node.parentId : ""
		}
		return false
	}

	function getNodePath(nodeId){
		let names = []
		let visitedIds = {}
		let currentId = nodeId
		while (currentId && !visitedIds[currentId] && topologyPage.topologyNodes[currentId]){
			visitedIds[currentId] = true
			let node = topologyPage.topologyNodes[currentId]
			names.unshift(node.name)
			currentId = node.parentId
		}
		return names.join(" / ")
	}

	// Groups the node can be moved to: not its current group, not the node itself and not its subgroups.
	function getTargetGroups(nodeId){
		let result = []
		let node = topologyPage.topologyNodes[nodeId]
		let currentParentId = node ? node.parentId : ""
		for (let candidateId in topologyPage.topologyNodes){
			if (!topologyPage.isGroupNode(candidateId) || candidateId === currentParentId)
				continue
			if (topologyPage.isNodeInside(candidateId, nodeId))
				continue
			result.push({"id": candidateId, "name": topologyPage.getNodePath(candidateId)})
		}
		result.sort(function(left, right){ return left.name.localeCompare(right.name) })
		return result
	}

	// Groups all the nodes can be moved to (getTargetGroups of every node).
	function getCommonTargetGroups(nodeIds){
		if (nodeIds.length === 0)
			return []
		let result = topologyPage.getTargetGroups(nodeIds[0])
		for (let i = 1; i < nodeIds.length && result.length > 0; ++i){
			let allowed = {}
			let groups = topologyPage.getTargetGroups(nodeIds[i])
			for (let k = 0; k < groups.length; ++k){
				allowed[groups[k].id] = true
			}
			result = result.filter(function(group){ return allowed[group.id] === true })
		}
		return result
	}

	// The same rule as getTargetGroups for one group (used while dragging a tile).
	function canAddToGroup(nodeId, groupId){
		if (!topologyPage.isGroupNode(groupId) || !topologyPage.topologyNodes[nodeId])
			return false
		if (topologyPage.topologyNodes[nodeId].parentId === groupId)
			return false
		return !topologyPage.isNodeInside(groupId, nodeId)
	}

	// Remembers all nodes of the view as they are in the draft (TopologyViewModel.currentState).
	function rebuildTopologyNodes(){
		let viewModel = topologyPage.topologyViewModel
		let state = viewModel.currentState
		let nodes = {}
		if (state){
			for (let id in state.nodes){
				let node = state.nodes[id]
				nodes[id] = {
					"id": id,
					"parentId": node.parentId,
					"nodeType": node.nodeType,
					"name": node.name,
					"agentId": node.agentId,
					"x": node.x,
					"y": node.y,
					"linkIds": topologyPage.readLinkIds(viewModel.getNodeItem(id))
				}
			}
		}
		topologyPage.topologyNodes = nodes
		topologyPage.availableServices = viewModel.availableServices
	}

	// Shows the nodes of the draft on the level (the children of its node). The model of the level is
	// replaced (two models are used in turn), so that the scheme resets its drag and drop state.
	function renderLevel(levelScheme){
		if (!levelScheme)
			return

		let viewModel = topologyPage.topologyViewModel
		let state = viewModel.currentState

		// The selected tiles stay selected (those still on the level); the commands are updated once at
		// the end: the temporary deselection must not hide and show them again (the command panel would jump).
		let previousNodeId = levelScheme.selectedNodeId
		let previousNodeIds = levelScheme.selectedIds.slice()
		let wasReselecting = topologyPage.reselecting
		topologyPage.reselecting = true
		levelScheme.selectedIndex = -1

		let nodesModel = levelScheme.objectsModel === levelScheme.firstModel ? levelScheme.secondModel : levelScheme.firstModel
		nodesModel.clear()
		if (state){
			for (let id in state.nodes){
				let node = state.nodes[id]
				if (node.parentId !== levelScheme.groupId)
					continue
				let item = viewModel.getNodeItem(id)
				if (!item)
					continue
				item.m_x = node.x
				item.m_y = node.y
				item.m_parentId = node.parentId
				if (node.nodeType === "GROUP"){
					item.m_mainText = node.name
				}
				// insertElement(): in JQML3 "BaseModel" here is its built-in QtQml.BaseModel, which has no appendElement().
				nodesModel.insertElement(nodesModel.count, item)
			}
		}

		// E.g. agents shown as composite nodes for the first time; the position is stored with the next save.
		for (let i = 0; i < nodesModel.count; ++i){
			let item = nodesModel.get(i).item
			if (item.m_x === 0 && item.m_y === 0){
				let position = topologyPage.findFreePosition(nodesModel)
				item.m_x = position.x
				item.m_y = position.y
			}
		}

		levelScheme.objectsModel = nodesModel
		levelScheme.extraLinks = topologyPage.computeLevelLinks(levelScheme.groupId)
		levelScheme.linkHints = topologyPage.computeLinkHints(levelScheme.groupId)
		levelScheme.setSelection(previousNodeIds, levelScheme.findModelIndex(previousNodeId))
		topologyPage.reselecting = wasReselecting
		levelScheme.requestPaint()
	}

	// The draft was changed (an edit, Undo/Redo, a reload from the server): the shown level is drawn again.
	function applyViewState(){
		topologyPage.rebuildTopologyNodes()

		// A shown composite node was removed (here or by another client): return to its parent level.
		let removedLevel = topologyPage.findRemovedLevel()
		if (removedLevel > 0){
			topologyPage.navigateToLevel(removedLevel - 1)
			return
		}

		let currentScheme = topologyPage.scheme
		if (!currentScheme)
			return

		topologyPage.renderLevel(currentScheme)
		topologyPage.updateLevelNames()
		topologyPage.applySelection(currentScheme)
		topologyPage.updateLinkInfo()
		topologyPage.updateCommandStates()
	}

	// IDs of the services the node is linked with (its m_links).
	function readLinkIds(item){
		let result = []
		let links = item ? item.m_links : null
		if (!links)
			return result
		for (let k = 0; k < links.count; ++k){
			let link = links.get(k).item
			if (link && link.m_id){
				result.push("" + link.m_id)
			}
		}
		return result
	}

	// The node shown on the level of groupId that is the node itself or contains it; "" if the node
	// is not inside that level.
	function findShownNode(nodeId, groupId){
		let visitedIds = {}
		let currentId = nodeId
		while (currentId && !visitedIds[currentId]){
			visitedIds[currentId] = true
			let node = topologyPage.topologyNodes[currentId]
			if (!node)
				return ""
			if (node.parentId === groupId)
				return currentId
			currentId = node.parentId
		}
		return ""
	}

	// Links of the shown level that the scheme does not draw by itself: a link between services in
	// different composite nodes (groups, agents) is drawn between these nodes, or between a node and
	// a service shown on the level. Direct links of two shown services are drawn by the scheme.
	function computeLevelLinks(groupId){
		let result = []
		let added = {}
		for (let sourceId in topologyPage.topologyNodes){
			let linkIds = topologyPage.topologyNodes[sourceId].linkIds
			if (!linkIds || linkIds.length === 0)
				continue
			let shownSourceId = topologyPage.findShownNode(sourceId, groupId)
			if (shownSourceId === "")
				continue
			for (let k = 0; k < linkIds.length; ++k){
				let targetId = linkIds[k]
				let shownTargetId = topologyPage.findShownNode(targetId, groupId)
				if (shownTargetId === "" || shownTargetId === shownSourceId)
					continue
				if (shownSourceId === sourceId && shownTargetId === targetId)
					continue
				// Same direction as the scheme: from the linked (target) node to the source node.
				let key = shownTargetId + "\n" + shownSourceId
				if (added[key])
					continue
				added[key] = true
				result.push({"fromId": shownTargetId, "toId": shownSourceId})
			}
		}
		return result
	}

	// Tooltips of the link icons of the level: {serviceId: text}. A shown service gets the icon if it
	// is linked (in either direction) with a service whose link cannot be drawn on this level: the
	// service is outside of the level or not in the current view.
	function computeLinkHints(groupId){
		// Services linked with each service, in both directions.
		let related = {}
		let addRelation = function(fromId, toId){
			if (!related[fromId]){
				related[fromId] = []
			}
			if (related[fromId].indexOf(toId) < 0){
				related[fromId].push(toId)
			}
		}
		for (let sourceId in topologyPage.topologyNodes){
			let linkIds = topologyPage.topologyNodes[sourceId].linkIds
			if (!linkIds)
				continue
			for (let k = 0; k < linkIds.length; ++k){
				addRelation(sourceId, linkIds[k])
				addRelation(linkIds[k], sourceId)
			}
		}

		let availableNames = {}
		for (let i = 0; i < topologyPage.availableServices.length; ++i){
			availableNames[topologyPage.availableServices[i].id] = topologyPage.availableServices[i].name
		}

		let hints = {}
		for (let nodeId in topologyPage.topologyNodes){
			let node = topologyPage.topologyNodes[nodeId]
			if (node.parentId !== groupId || node.nodeType !== "SERVICE" || !related[nodeId])
				continue
			let lines = []
			for (let k = 0; k < related[nodeId].length; ++k){
				let otherId = related[nodeId][k]
				if (topologyPage.findShownNode(otherId, groupId) !== "")
					continue
				let other = topologyPage.topologyNodes[otherId]
				if (other){
					let groupPath = topologyPage.getNodePath(other.parentId)
					lines.push(groupPath !== "" ? groupPath + " / " + other.name : "" + other.name)
				}
				else if (availableNames[otherId] !== undefined){
					lines.push(availableNames[otherId] + " (" + qsTr("not in this view") + ")")
				}
			}
			if (lines.length > 0){
				lines.sort()
				hints[nodeId] = qsTr("Linked with:") + "\n" + lines.join("\n")
			}
		}
		return hints
	}
	function setCommandState(commandId, isEnabled){
		let commandsController = topologyPage.commandsController
		if (!commandsController)
			return
		commandsController.setCommandIsEnabled(commandId, isEnabled)
		commandsController.setCommandVisible(commandId, isEnabled)
	}

	function updateCommandStates(){
		if (!topologyPage.commandsController)
			return

		let item = topologyPage.selectedNode()
		let isComposite = topologyPage.isCompositeNode(item)
		let serviceSelected = item !== null && !isComposite
		let agentOnline = item !== null && item.m_agentOnline !== false
		let status = serviceSelected ? topologyPage.normalizeServiceStatus(item.m_status) : ""
		let canStartStop = serviceSelected && agentOnline

		topologyPage.setCommandState("Start", canStartStop && status === "NOT_RUNNING")
		topologyPage.setCommandState("Stop", canStartStop && status === "RUNNING")
		// Groups are renamed on the server; services through their agent, which must be online.
		// Agents are renamed in the agent editor.
		topologyPage.setCommandState("Rename", item !== null && (isComposite ? topologyPage.isGroupNode(item.m_id) : agentOnline))
		// Remove: a group (agents cannot be removed here) or, in a user-defined view, a service from
		// this view only. Remove Service deletes the service physically; it is available only in the
		// "Agents" view (inside an agent or a group; the agent app shows the services of its agent).
		// Delete requires a live agent — mirror-only delete comes back on reconnect.
		topologyPage.setCommandState("Remove", isComposite ? topologyPage.isGroupNode(item.m_id) : serviceSelected && topologyPage.viewId !== "")
		topologyPage.setCommandState("RemoveService", serviceSelected && agentOnline && topologyPage.viewId === "")
		// A user-defined view is created empty; services are added to the current level (root or group).
		topologyPage.setCommandState("AddToView", topologyPage.viewId !== "" && topologyPage.availableServices.length > 0
				&& (topologyPage.groupId === "" || topologyPage.isGroupNode(topologyPage.groupId)))
		// Groups live at the root level or inside other groups; an agent contains only its services.
		topologyPage.setCommandState("NewGroup", topologyPage.groupId === "" || topologyPage.isGroupNode(topologyPage.groupId))
		// Group membership commands are applied to all selected nodes.
		let selectedIds = topologyPage.selectedNodeIds()
		// Without a group to add the nodes to, a new group is created for them.
		topologyPage.setCommandState("AddToGroup", selectedIds.length > 0)
		// "Move to parent": the nodes go one level up, into the group containing the current group.
		topologyPage.setCommandState("RemoveFromGroup", selectedIds.length > 0 && topologyPage.isGroupNode(topologyPage.groupId))
		// Ungroup: the selected groups are removed, their content goes to the level of the groups.
		topologyPage.setCommandState("Ungroup", topologyPage.selectedGroupIds().length > 0)
		// Changes are collected in the local draft of the view; Undo/Redo step through the draft, Save sends
		// the whole view to the server. The three commands are always shown, only their enabled state changes:
		// Undo/Redo while there is a step to undo/redo, Save while the draft has unsaved changes.
		let viewModel = topologyPage.topologyViewModel
		topologyPage.commandsController.setCommandIsEnabled("Undo", viewModel.canUndo)
		topologyPage.commandsController.setCommandIsEnabled("Redo", viewModel.canRedo)
		topologyPage.commandsController.setCommandIsEnabled("Save", viewModel.canSave)
		topologyPage.commandsController.setCommandVisible("Undo", true)
		topologyPage.commandsController.setCommandVisible("Redo", true)
		topologyPage.commandsController.setCommandVisible("Save", true)
	}

	property TreeItemModel contextMenuModel: TreeItemModel {}

	// Context menu of the selected tile: the active commands of the toolbar, handled by the same delegate.
	function openContextMenu(x, y){
		let commandsController = topologyPage.commandsController
		if (!commandsController || !topologyPage.commandsDelegate)
			return

		let commands = [
			{"id": "Remove", "name": qsTr("Remove"), "icon": "Icons/Delete"},
			{"id": "RemoveService", "name": qsTr("Remove service"), "icon": "Icons/Delete"},
			{"id": "Rename", "name": qsTr("Rename"), "icon": "Icons/Edit"},
			{"id": "AddToGroup", "name": qsTr("Add to group"), "icon": "Icons/Link"},
			{"id": "RemoveFromGroup", "name": qsTr("Move to parent"), "icon": "Icons/Unlink"},
			{"id": "Ungroup", "name": qsTr("Ungroup"), "icon": "Icons/FolderOpened"},
			{"id": "Start", "name": qsTr("Start service"), "icon": "Icons/Play"},
			{"id": "Stop", "name": qsTr("Stop service"), "icon": "Icons/Stop"}
		]

		topologyPage.contextMenuModel.clear()
		for (let i = 0; i < commands.length; ++i){
			if (!commandsController.commandIsEnabled(commands[i].id))
				continue
			let index = topologyPage.contextMenuModel.insertNewItem()
			topologyPage.contextMenuModel.setData("id", commands[i].id, index)
			topologyPage.contextMenuModel.setData("name", commands[i].name, index)
			topologyPage.contextMenuModel.setData("icon", commands[i].icon, index)
		}

		let count = topologyPage.contextMenuModel.getItemsCount()
		if (count === 0)
			return

		// Opened upwards when it does not fit below the cursor.
		let menuHeight = count * (Style.controlHeightS + Style.spacingXS) + 2 * Style.marginXS
		let bottom = topologyPage.mapToItem(null, 0, topologyPage.height).y
		let menuY = y + menuHeight > bottom ? Math.max(0, y - menuHeight) : y

		ModalDialogManager.openDialog(topologyContextMenuComp, {"x": x, "y": menuY, "model": topologyPage.contextMenuModel, "shownItemsCount": count})
	}

	function openServiceDocument(serviceId, serviceName, agentId){
		if (!topologyPage.documentManager)
			return
		topologyPage.documentAgentId = agentId ? agentId : ""
		topologyPage.documentManager.openDocument(serviceId, "Service", serviceName)
	}

	function createServiceDocument(agentId){
		if (!topologyPage.documentManager)
			return
		topologyPage.documentAgentId = agentId ? agentId : ""
		topologyPage.documentManager.insertNewDocument("Service", qsTr("New service"))
	}

	function updateCurrentScheme(){
		let currentScheme = schemeStack.currentPage()
		if (!currentScheme || currentScheme === topologyPage.scheme)
			return

		topologyPage.scheme = currentScheme
		// The draft may have been changed while the level was hidden.
		topologyPage.renderLevel(currentScheme)
		topologyPage.applySelection(currentScheme)
		topologyPage.updateLinkInfo()
		if (topologyPage.commandsController){
			topologyPage.commandsController.setToggled("AutoFit", currentScheme.autoFit)
		}
	}

	// Goes inside a composite node: its content becomes the next level of the navigation.
	function openLevel(nodeId, name){
		let levelNodeIds = topologyPage.levelNodeIds.slice()
		levelNodeIds.push(nodeId)
		topologyPage.levelNodeIds = levelNodeIds

		stackHeader.addHeader(nodeId, name)
		schemeStack.addPage(schemeLevelComp)
		schemeStack.setCurrentIndex(schemeStack.count - 1)
		topologyPage.updateCurrentScheme()

		topologyPage.topologyViewModel.loadTopology()
	}

	// Returns to the level with the given index of the navigation panel; the deeper levels are closed.
	function navigateToLevel(index){
		if (index < 0 || index >= schemeStack.count - 1)
			return

		schemeStack.setCurrentIndex(index)
		while (schemeStack.count > index + 1){
			schemeStack.removePage(schemeStack.count - 1)
			stackHeader.popHeader()
		}
		topologyPage.levelNodeIds = topologyPage.levelNodeIds.slice(0, index + 1)
		topologyPage.updateCurrentScheme()

		// The upper levels were not updated while hidden.
		topologyPage.topologyViewModel.loadTopology()
	}

	// Index of the first level whose node no longer exists in the draft (removed, undone or removed by
	// another client), -1 if all exist. Nothing is removed before the view is loaded.
	function findRemovedLevel(){
		if (!topologyPage.topologyViewModel.currentState)
			return -1
		for (let i = 1; i < topologyPage.levelNodeIds.length; ++i){
			if (!topologyPage.topologyNodes[topologyPage.levelNodeIds[i]])
				return i
		}
		return -1
	}

	// Group names can be changed by other clients.
	function updateLevelNames(){
		for (let i = 1; i < topologyPage.levelNodeIds.length; ++i){
			let node = topologyPage.topologyNodes[topologyPage.levelNodeIds[i]]
			if (node && stackHeader.getHeaderName(node.id) !== node.name){
				stackHeader.setHeaderName(node.id, node.name)
			}
		}
	}

	// --- Views ---

	// Filter of the views panel: a view matches by its name or by the names of its services and groups.
	property string viewsFilterText: ""
	readonly property int matchingViewsCount: {
		let count = 0
		for (let i = 0; i < topologyPage.viewsModel.count; ++i){
			if (topologyPage.viewMatchesFilter(topologyPage.viewsModel.get(i).searchText, topologyPage.viewsFilterText)){
				++count
			}
		}
		return count
	}

	// Lower-case text searched by the filter: the view name and the names of its content, one per line.
	function makeViewSearchText(viewName, nodeNames){
		let names = [viewName ? "" + viewName : ""]
		if (nodeNames){
			for (let i = 0; i < nodeNames.length; ++i){
				names.push("" + nodeNames[i])
			}
		}
		return names.join("\n").toLowerCase()
	}

	function viewMatchesFilter(searchText, filterText){
		let filter = filterText ? filterText.trim().toLowerCase() : ""
		return filter === "" || (searchText ? searchText : "").indexOf(filter) >= 0
	}

	function getViewName(viewId){
		for (let i = 0; i < topologyPage.viewsModel.count; ++i){
			if (topologyPage.viewsModel.get(i).id === viewId){
				return topologyPage.viewsModel.get(i).name
			}
		}
		return ""
	}

	// views: [{id, name, nodeNames}] of the user-defined views; the default view is always the first, pinned item.
	// agentsViewNodeNames: names of the agents, services and groups of the default view.
	function fillViews(views, agentsViewNodeNames){
		topologyPage.viewsModel.clear()
		topologyPage.viewsModel.append({"id": "", "name": qsTr("Agents"), "pinned": true,
				"searchText": topologyPage.makeViewSearchText(qsTr("Agents"), agentsViewNodeNames)})
		let currentViewExists = topologyPage.viewId === ""
		for (let i = 0; i < views.length; ++i){
			topologyPage.viewsModel.append({"id": views[i].id, "name": views[i].name, "pinned": false,
					"searchText": topologyPage.makeViewSearchText(views[i].name, views[i].nodeNames)})
			if (views[i].id === topologyPage.viewId){
				currentViewExists = true
			}
		}

		// The current view was removed by another client: its draft is dropped.
		if (!currentViewExists){
			topologyPage.showView("")
			return
		}

		stackHeader.setHeaderName("", topologyPage.getViewName(topologyPage.viewId))
	}

	// Shows another view. Unsaved changes of the current view are saved or discarded first (asked).
	function selectView(viewId){
		if (viewId === topologyPage.viewId){
			return
		}

		if (topologyPage.topologyViewModel.dirty){
			ModalDialogManager.openDialog(unsavedChangesDialogComp, {
					"viewName": topologyPage.getViewName(topologyPage.viewId),
					"continueAction": function(){ topologyPage.showView(viewId) }})
			return
		}

		topologyPage.showView(viewId)
	}

	// Shows the root level of the view; the levels opened in the previous view are closed and the draft
	// of the previous view is dropped.
	function showView(viewId){
		if (viewId === topologyPage.viewId){
			return
		}

		topologyPage.clearServiceSelection()
		topologyPage.availableServices = []
		topologyPage.updateCommandStates()

		while (schemeStack.count > 1){
			schemeStack.removePage(schemeStack.count - 1)
			stackHeader.popHeader()
		}
		schemeStack.setCurrentIndex(0)
		topologyPage.levelNodeIds = [""]
		topologyPage.updateCurrentScheme()

		stackHeader.setHeaderName("", topologyPage.getViewName(viewId))

		topologyPage.topologyViewModel.setView(viewId)
	}

	function requestAddView(){
		ModalDialogManager.openDialog(newViewDialogComp, {})
	}

	function requestRemoveView(viewId){
		// The default view cannot be removed.
		if (viewId === ""){
			return
		}
		ModalDialogManager.openDialog(removeViewDialogComp, {"viewId": viewId, "viewName": topologyPage.getViewName(viewId)})
	}

	property TreeItemModel viewContextMenuModel: TreeItemModel {}

	// Context menu of a view item: Remove only. The pinned default view has no commands, so no menu is opened for it.
	function openViewContextMenu(viewId, x, y){
		if (viewId === ""){
			return
		}

		topologyPage.viewContextMenuModel.clear()

		let index = topologyPage.viewContextMenuModel.insertNewItem()
		topologyPage.viewContextMenuModel.setData("id", "RemoveView", index)
		topologyPage.viewContextMenuModel.setData("name", qsTr("Remove"), index)
		topologyPage.viewContextMenuModel.setData("icon", "Icons/Delete", index)

		ModalDialogManager.openDialog(viewContextMenuComp, {"x": x, "y": y, "model": topologyPage.viewContextMenuModel, "contextViewId": viewId})
	}

	// Nodes at (0, 0) have no stored position yet and do not occupy space.
	function findFreePosition(nodesModel){
		let width = 280
		let height = 60
		let margin = 30
		let count = nodesModel ? nodesModel.count : 0
		for (let y = 40; y + height < 768; y += height + margin){
			for (let x = 40; x + width < 1024; x += width + margin){
				let isFree = true
				for (let i = 0; i < count && isFree; ++i){
					let item = nodesModel.get(i).item
					if (!item || (item.m_x === 0 && item.m_y === 0))
						continue
					let itemWidth = item.m_width ? item.m_width : width
					isFree = x + width + margin <= item.m_x || item.m_x + itemWidth + margin <= x
							|| y + height + margin <= item.m_y || item.m_y + height + margin <= y
				}
				if (isFree)
					return Qt.point(x, y)
			}
		}
		return Qt.point(40, 40)
	}

	function requestNewGroup(){
		ModalDialogManager.openDialog(newGroupDialogComp, {})
	}

	// Adds services (chosen from the services not yet in the view) to the current level of the user-defined view.
	function requestAddToView(){
		if (topologyPage.viewId === "" || topologyPage.availableServices.length === 0)
			return
		let parentId = topologyPage.isGroupNode(topologyPage.groupId) ? topologyPage.groupId : ""
		ModalDialogManager.openDialog(addToViewDialogComp, {"parentId": parentId, "services": topologyPage.availableServices.slice()})
	}

	// Moves all selected nodes into a group chosen in the dialog. If there is no group they can be added
	// to, the dialog of a new group is opened; the nodes are moved into the created group.
	function requestAddToGroup(){
		let nodeIds = topologyPage.selectedNodeIds()
		if (nodeIds.length === 0)
			return
		let targetGroups = topologyPage.getCommonTargetGroups(nodeIds)
		if (targetGroups.length === 0){
			ModalDialogManager.openDialog(newGroupDialogComp, {"nodeIds": nodeIds})
			return
		}
		ModalDialogManager.openDialog(addToGroupDialogComp, {"nodeIds": nodeIds, "targetGroups": targetGroups})
	}

	// Position of a new group created for the nodes of the shown level: the place of the top left node
	// (the nodes leave the level); a free place if the nodes are not on the level.
	function findGroupPosition(nodeIds){
		let currentScheme = topologyPage.scheme
		let position = null
		for (let i = 0; currentScheme && i < nodeIds.length; ++i){
			let index = currentScheme.findModelIndex(nodeIds[i])
			if (index < 0)
				continue
			let item = currentScheme.objectsModel.get(index).item
			if (!position || item.m_y < position.y || (item.m_y === position.y && item.m_x < position.x)){
				position = Qt.point(item.m_x, item.m_y)
			}
		}
		return position ? position : topologyPage.findFreePosition(currentScheme ? currentScheme.objectsModel : null)
	}

	// "Move to parent": moves the selected nodes one level up, into the group containing the current
	// group, or back to their default place.
	function requestRemoveFromGroup(){
		let nodeIds = topologyPage.selectedNodeIds()
		if (nodeIds.length === 0 || !topologyPage.isGroupNode(topologyPage.groupId))
			return
		let currentGroup = topologyPage.topologyNodes[topologyPage.groupId]
		let targetGroupId = topologyPage.isGroupNode(currentGroup.parentId) ? currentGroup.parentId : ""
		topologyPage.topologyViewModel.moveNodes(nodeIds, targetGroupId)
	}

	// Ungroup: removes the selected groups; their content goes to the level of the groups. One step of
	// the history, undone at once.
	function requestUngroup(){
		let groupIds = topologyPage.selectedGroupIds()
		if (groupIds.length === 0)
			return
		topologyPage.clearServiceSelection()
		topologyPage.topologyViewModel.removeGroups(groupIds)
	}

	// Takes the layout of the level into the draft after a move (one step of the history). Tiles placed
	// on a free place when shown (no stored position) get this position as well.
	function storeLevelLayout(levelScheme){
		let state = topologyPage.topologyViewModel.currentState
		let nodesModel = levelScheme.objectsModel
		if (!state || !nodesModel)
			return
		let positions = []
		for (let i = 0; i < nodesModel.count; ++i){
			let item = nodesModel.get(i).item
			let node = item ? state.nodes[item.m_id] : null
			if (node && (node.x !== item.m_x || node.y !== item.m_y)){
				positions.push({"id": item.m_id, "x": item.m_x, "y": item.m_y})
			}
		}
		topologyPage.topologyViewModel.setPositions(positions)
	}

	// Renames the selected group or service (F2, toolbar or context menu).
	function requestRename(){
		let item = topologyPage.selectedNode()
		if (!item || !topologyPage.commandsController || !topologyPage.commandsController.commandIsEnabled("Rename"))
			return

		let isGroup = topologyPage.isCompositeNode(item)
		let name = item.m_mainText ? "" + item.m_mainText : ""
		// A service is shown as "<service name>@<agent name>".
		if (!isGroup && name.lastIndexOf("@") > 0){
			name = name.substring(0, name.lastIndexOf("@"))
		}

		ModalDialogManager.openDialog(renameDialogComp, {
				"nodeId": item.m_id,
				"agentId": isGroup ? "" : item.m_agentId,
				"isGroup": isGroup,
				"oldName": name,
				"inputValue": name,
				"title": isGroup ? qsTr("Rename group") : qsTr("Rename service")})
	}

	// Remove: deletes the selected group or removes the selected service from the current user-defined view.
	function requestRemoveSelected(){
		let item = topologyPage.selectedNode()
		if (!item || !topologyPage.commandsController || !topologyPage.commandsController.commandIsEnabled("Remove"))
			return
		if (topologyPage.isCompositeNode(item)){
			if (topologyPage.isGroupNode(item.m_id)){
				ModalDialogManager.openDialog(removeGroupDialogComp, {"groupId": item.m_id})
			}
			return
		}
		if (topologyPage.viewId !== ""){
			ModalDialogManager.openDialog(removeFromViewDialogComp, {"nodeId": item.m_id})
		}
	}

	// Remove Service: deletes the selected service physically (on its agent).
	function requestRemoveService(){
		let item = topologyPage.selectedNode()
		if (!item || topologyPage.isCompositeNode(item) || !topologyPage.commandsController
				|| !topologyPage.commandsController.commandIsEnabled("RemoveService"))
			return
		if (!topologyPage.canRemoveSelectedService()){
			topologyPage.showCannotRemoveWhileDisconnected()
			return
		}
		// A service is shown as "<service name>@<agent name>".
		let text = item.m_mainText ? "" + item.m_mainText : ""
		let separatorIndex = text.lastIndexOf("@")
		let agentNode = topologyPage.topologyNodes[item.m_agentId]
		ModalDialogManager.openDialog(removeServiceDialogComp, {
				"serviceId": item.m_id,
				"serviceName": separatorIndex > 0 ? text.substring(0, separatorIndex) : text,
				"agentName": agentNode ? "" + agentNode.name : (separatorIndex > 0 ? text.substring(separatorIndex + 1) : "")})
	}

	// Delete key: the removal command available for the selected tile.
	function requestDelete(){
		let commandsController = topologyPage.commandsController
		if (!commandsController)
			return
		if (commandsController.commandIsEnabled("Remove")){
			topologyPage.requestRemoveSelected()
		}
		else if (commandsController.commandIsEnabled("RemoveService")){
			topologyPage.requestRemoveService()
		}
	}

	// Topology.Service.agentOnline — false when the owning agent is disconnected.
	function isSelectedAgentOnline(){
		let item = topologyPage.selectedNode()
		if (!item)
			return false
		// Treat missing field as online (legacy payloads); only explicit false blocks delete.
		return item.m_agentOnline !== false
	}

	function canRemoveSelectedService(){
		return topologyPage.selectedService !== "" && topologyPage.isSelectedAgentOnline()
	}

	function showCannotRemoveWhileDisconnected(){
		ModalDialogManager.showErrorDialog(
					qsTr("Cannot delete the service while the agent is disconnected. Reconnect the agent and try again."))
	}

	Component.onCompleted: {
		if (documentManager){
			documentManager.registerDocumentView("Service", serviceEditorComp);
			documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
			documentManager.registerDocumentValidator("Service", serviceValidatorComp);
		}
		
		topologyViewModel.loadTopology()
		topologyViewModel.loadViews()
	}

	Connections {
		target: topologyPage.topologyViewModel
		// Signal parameters must be listed in function onXxx(...) — without them
		// the body reads outer/undefined names (data/topologySdlObject) and crashes.
		function onViewStateChanged() {
			topologyPage.applyViewState()
		}
		function onSavingChanged() {
			topologyPage.updateCommandStates()
		}
		// Steps of the views (create/remove) change the history without changing the draft.
		function onCanUndoChanged() {
			topologyPage.updateCommandStates()
		}
		function onCanRedoChanged() {
			topologyPage.updateCommandStates()
		}
		function onServiceStatusMessage(data) {
			topologyPage.applyLiveServiceStatus(data)
		}
		function onGroupOperationFailed(message) {
			ModalDialogManager.showErrorDialog(message !== "" ? message : qsTr("Unable to change the topology groups"))
		}
		function onViewsLoaded(views, agentsViewNodeNames) {
			topologyPage.fillViews(views, agentsViewNodeNames)
		}
		function onViewCreated(viewId) {
			topologyPage.selectView(viewId)
		}
		// Undo of a removal / Redo of a creation: the view is shown again.
		function onViewRestored(viewId) {
			topologyPage.selectView(viewId)
		}
		// The draft of a removed view is dropped.
		function onViewRemoved(viewId) {
			if (viewId === topologyPage.viewId){
				topologyPage.showView("")
			}
		}
	}

	// Undo/Redo of the draft of the view; disabled while a dialog is open. Ctrl+S (Save) is handled by
	// the base commands delegate (commandHandle("Save")).
	Shortcut {
		sequence: "Ctrl+Z"
		enabled: topologyPage.visible && ModalDialogManager.count === 0
		onActivated: {
			topologyPage.topologyViewModel.undo()
		}
	}

	Shortcut {
		sequence: "Ctrl+Shift+Z"
		enabled: topologyPage.visible && ModalDialogManager.count === 0
		onActivated: {
			topologyPage.topologyViewModel.redo()
		}
	}

	Shortcut {
		sequence: "Ctrl+Y"
		enabled: topologyPage.visible && ModalDialogManager.count === 0
		onActivated: {
			topologyPage.topologyViewModel.redo()
		}
	}
		commandsControllerComp: Component {GqlBasedCommandsController {
			typeId: "Topology"
			
			onCommandsChanged: {
				// Without a Save command (agent app) every change of the topology is saved at once.
				topologyPage.topologyViewModel.autoSave = !commandExists("Save")
				// "New service" is temporarily hidden; the command and its handler are kept.
				setCommandIsEnabled("New", true)
				setCommandVisible("New", false)
				setIsToggleable("AutoFit", true);
				setToggled("AutoFit", topologyPage.scheme ? topologyPage.scheme.autoFit : true);
				topologyPage.updateCommandStates()
			}
		}
	}
	
	onVisibleChanged: {
		if (topologyPage.visible){
			topologyViewModel.loadTopology()
			topologyViewModel.loadViews()
		}
	}

	// A hidden views panel must not leave the page in a user-defined view.
	Connections {
		target: topologyPage.topologyViewModel
		function onViewsSupportedChanged() {
			if (!topologyPage.topologyViewModel.viewsSupported){
				topologyPage.showView("")
			}
		}
	}

	function applyLiveServiceStatus(dataModel) {
		// Subscription payload is a TreeItemModel (has getData). Plain objects / missing
		// Connections args must not throw into the UI event loop.
		if (!dataModel || typeof dataModel.getData !== "function"){
			return
		}

		// Only the shown level is updated; the others are reloaded when they are shown again.
		let scheme = topologyPage.scheme
		if (!scheme){
			return
		}

		let serviceId = dataModel.getData("serviceid")
		if (!serviceId){
			return
		}
		let serviceStatus = topologyPage.normalizeServiceStatus(dataModel.getData(ServiceStatus.s_Key))
		let dependencyStatus

		let index = scheme.findModelIndex(serviceId);
		if (index < 0){
			return;
		}

		let item = scheme.objectsModel.get(index).item

		if (item.m_agentOnline === false){
			item.m_status = "UNDEFINED"
			item.m_icon1 = "Icons/Alert"
			item.m_icon2 = "Icons/Warning"
			if (index === scheme.selectedIndex){
				topologyPage.updateCommandStates()
			}
			scheme.requestPaint()
			return
		}

		item.m_status = serviceStatus;
		if (serviceStatus === "RUNNING"){
			item.m_icon1 = "Icons/Running";
		}
		else if (serviceStatus === "NOT_RUNNING"){
			item.m_icon1 = "Icons/Stopped";
		}
		else if (serviceStatus === "STARTING" || serviceStatus === "STOPPING"){
			if (item.m_icon1 === "" || item.m_icon1 === "Icons/Alert"){
				item.m_icon1 = "Icons/Stopped";
			}
		}
		else if (serviceStatus === "UNDEFINED"){
			item.m_icon1 = "Icons/Alert";
		}

		// STARTING / STOPPING enable neither Start nor Stop.
		if (index === scheme.selectedIndex){
			topologyPage.updateCommandStates()
		}

		serviceStatus = item.m_status;

		let dependencyStatusModel = dataModel.containsKey && dataModel.containsKey(DependencyStatus.s_Key)
					? dataModel.getData(DependencyStatus.s_Key)
					: null
		if (!dependencyStatusModel || typeof dependencyStatusModel.getItemsCount !== "function"){
			scheme.requestPaint()
			return
		}
		for (let i = 0; i < dependencyStatusModel.getItemsCount(); i++){
			serviceId = dependencyStatusModel.getData("id", i);
			index = scheme.findModelIndex(serviceId);
			dependencyStatus = topologyPage.normalizeServiceStatus(dependencyStatusModel.getData(DependencyStatus.s_Key, i))
			if (index < 0){
				continue
			}

			let serviceItem = scheme.objectsModel.get(index).item;
			if (serviceItem.m_agentOnline === false){
				serviceItem.m_icon2 = "Icons/Warning"
				continue
			}
			if (dependencyStatus === "NOT_RUNNING"){
				serviceItem.m_icon2 = "Icons/Error"
			}
			else if (dependencyStatus === "UNDEFINED"){
				serviceItem.m_icon2 = "Icons/Warning"
			}
			else {
				serviceItem.m_icon2 = ""
			}

			if (serviceStatus !== "RUNNING"){
				item.m_icon2 = ""
			}

			if (serviceId === scheme.selectedService){
				topologyPage.refreshSelectedMetaInfo()
			}
		}

		scheme.requestPaint()
	}
		
	commandsDelegateComp: Component {ServiceCollectionViewCommandsDelegate {
			id: serviceCommandsDelegate
			collectionView: topologyPage
			view: topologyPage;
			documentTypeId: "Service"
			documentManager: topologyPage.documentManager
			removeDialogTitle: qsTr("Deleting a service")
			removeMessage: qsTr("Delete the selected service?")

			// The base implementation reads the selection from collectionView.table, which the
			// topology scheme does not have.
			function commandHandle(commandId, params){
				let commandsController = topologyPage.commandsController
				if (commandsController && commandsController.commandIsEnabled(commandId)){
					if (commandId === "New"){
						onNew(params)
					}
					else if (commandId === "Remove"){
						onRemove(params)
					}
					else if (commandId === "RemoveService"){
						topologyPage.requestRemoveService()
					}
					else if (commandId === "Save"){
						topologyPage.topologyViewModel.save()
					}
					else if (commandId === "Undo"){
						topologyPage.topologyViewModel.undo()
					}
					else if (commandId === "Redo"){
						topologyPage.topologyViewModel.redo()
					}
					else if (commandId === "AddToView"){
						topologyPage.requestAddToView()
					}
					else if (commandId === "NewGroup"){
						topologyPage.requestNewGroup()
					}
					else if (commandId === "AddToGroup"){
						topologyPage.requestAddToGroup()
					}
					else if (commandId === "RemoveFromGroup"){
						topologyPage.requestRemoveFromGroup()
					}
					else if (commandId === "Ungroup"){
						topologyPage.requestUngroup()
					}
					else if (commandId === "Rename"){
						topologyPage.requestRename()
					}
				}

				commandActivated(commandId)
			}

			// Used by the base delegate for Start/Stop while a service operation runs.
			function setCommandEnabled(commandId, isEnabled){
				topologyPage.setCommandState(commandId, isEnabled)
			}
			
			onCommandActivated: {
				let scheme = topologyPage.scheme
				if (!scheme){
					return
				}
				if (commandId === "ZoomIn"){
					scheme.setAutoFit(false);
					
					scheme.zoomIn();
				}
				else if (commandId === "ZoomOut"){
					scheme.setAutoFit(false);
					
					scheme.zoomOut();
				}
				else if (commandId === "ZoomReset"){
					scheme.resetZoom();
				}
				else if (commandId === "AutoFit"){
					scheme.setAutoFit(!scheme.autoFit);
				}
			}
			
			function onStart(){
				let item = topologyPage.selectedNode()
				if (item && !topologyPage.isCompositeNode(item)){
					startService(item.m_id);
				}
			}
			
			function onStop(){
				let item = topologyPage.selectedNode()
				if (item && !topologyPage.isCompositeNode(item)){
					stopService(item.m_id);
				}
			}
			
			function onNew(){
				// Inside an agent the target agent is known.
				let currentNode = topologyPage.topologyNodes[topologyPage.groupId]
				if (currentNode && currentNode.nodeType === "AGENT"){
					topologyPage.createServiceDocument(currentNode.id)
					return
				}

				// Server app has an "Agents" document service → pick target agent.
				// Agent app has no Agents page → create service locally without clientid.
				if (MainDocumentService.getDocumentService("Agents")){
					ModalDialogManager.openDialog(newServiceAgentDialogComp, {})
					return
				}

				MainDocumentService.tryRegisterDocumentService("Agents", function(result){
					if (result){
						ModalDialogManager.openDialog(newServiceAgentDialogComp, {})
					}
					else{
						topologyPage.createServiceDocument("")
					}
				})
			}

			function onRemove(){
				topologyPage.requestRemoveSelected()
			}

			function getHeaders(){
				return topologyPage.getHeaders();
			}

			Component {
				id: newServiceAgentDialogComp

				Dialog {
					id: newServiceAgentDialog
					width: Style.sizeHintM
					title: qsTr("Select target agent")

					property string chosenAgentId: ""

					Component.onCompleted: {
						addButton(Enums.apply, qsTr("Select"), false)
						addButton(Enums.cancel, qsTr("Cancel"), true)
					}

					onFinished: {
						if (buttonId === Enums.apply && chosenAgentId !== ""){
							topologyPage.createServiceDocument(chosenAgentId)
						}
					}

					contentComp: Component {
						Item {
							width: newServiceAgentDialog.width
							height: Style.controlHeightM + 2 * Style.marginL

							ComboBoxGqlSimple {
								id: agentCombo
								anchors.centerIn: parent
								width: parent.width - 2 * Style.marginL
								height: Style.controlHeightM
								gqlCommandId: "AgentsList"
								fields: ["id", "computerName"]
								nameId: "computerName"

								onModelChanged: {
									if (model && model.getItemsCount() > 0){
										currentIndex = 0
									}
								}

								onCurrentIndexChanged: {
									if (currentIndex >= 0 && model && model.containsKey("id", currentIndex)){
										newServiceAgentDialog.chosenAgentId = model.getData("id", currentIndex)
										newServiceAgentDialog.setButtonEnabled(Enums.apply, true)
									}
									else{
										newServiceAgentDialog.chosenAgentId = ""
										newServiceAgentDialog.setButtonEnabled(Enums.apply, false)
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	// Applies the selection of the shown level to the page (meta information, commands, headers).
	function applySelection(levelScheme){
		if (!levelScheme || levelScheme !== topologyPage.scheme || topologyPage.reselecting){
			return
		}

		let item = topologyPage.selectedNode()
		let isComposite = topologyPage.isCompositeNode(item)

		topologyPage.selectedAgentId = item !== null && !isComposite ? item.m_agentId : ""
		// Meta information is provided for services only.
		metaInfo.contentVisible = item !== null && !isComposite
		topologyPage.refreshSelectedMetaInfo()

		topologyPage.updateCommandStates()
	}

	// Views (ways of grouping): styled as the page list of MultiPageView (General / Services /
	// Agent Log / Terminal of AgentEditor) - no background of its own, a thin separator on the right.
	Item {
		id: viewsPanel

		objectName: "TopologyViewsPanel"

		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		width: visible ? Style.sizeHintXXS + 2 * Style.marginM + viewsSeparator.width : 0
		// Hidden where views are not supported (agent app).
		visible: topologyPage.topologyViewModel.viewsSupported

		Rectangle {
			id: viewsSeparator

			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right

			width: 1
			color: Style.borderColor
			opacity: 0.5
		}

		Item {
			id: viewsContent

			anchors.top: parent.top
			anchors.topMargin: Style.marginM
			anchors.left: parent.left
			anchors.leftMargin: Style.marginM
			anchors.bottom: parent.bottom

			width: Style.sizeHintXXS

			// Title line: "Views" with the Add / Remove icon buttons on the right.
			Item {
				id: viewsHeader

				anchors.top: parent.top
				anchors.left: parent.left
				anchors.leftMargin: Style.marginM
				anchors.right: parent.right

				height: Style.controlHeightM

				BaseText {
					id: viewsTitle

					anchors.verticalCenter: parent.verticalCenter
					anchors.left: parent.left
					anchors.right: viewsButtons.left
					anchors.rightMargin: Style.marginS

					text: qsTr("Views")
					color: Style.subtitleColor
					font.family: Style.fontFamilyBold
					font.pixelSize: Style.fontSizeM
					elide: Text.ElideRight
				}

				Row {
					id: viewsButtons

					anchors.verticalCenter: parent.verticalCenter
					anchors.right: parent.right

					spacing: Style.spacingXS

					Button {
						id: addViewButton

						objectName: "TopologyAddViewButton"
						width: Style.controlHeightM
						height: Style.controlHeightM
						tooltipText: qsTr("Add view")
						iconSource: "qrc:/" + Style.getIconPath("Icons/Add", Icon.State.On,
									enabled ? Icon.Mode.Normal : Icon.Mode.Disabled)
						decorator: Component {
							ButtonDecorator {
								color: parent.hovered ? Style.alternateBaseColor : "transparent"
								radius: Style.radiusM
								border.width: 0
							}
						}
						onClicked: {
							topologyPage.requestAddView()
						}
					}

					Button {
						id: removeViewButton

						objectName: "TopologyRemoveViewButton"
						width: Style.controlHeightM
						height: Style.controlHeightM
						tooltipText: qsTr("Remove view")
						// The pinned default view cannot be removed.
						enabled: topologyPage.viewId !== ""
						iconSource: "qrc:/" + Style.getIconPath("Icons/Delete", Icon.State.On,
									enabled ? Icon.Mode.Normal : Icon.Mode.Disabled)
						decorator: Component {
							ButtonDecorator {
								color: parent.hovered ? Style.alternateBaseColor : "transparent"
								radius: Style.radiusM
								border.width: 0
							}
						}
						onClicked: {
							topologyPage.requestRemoveView(topologyPage.viewId)
						}
					}
				}
			}

			// Finds a view by its name or by the names of its services and groups.
			CustomTextField {
				id: viewsFilterField

				objectName: "TopologyViewsFilter"

				// Same horizontal bounds as the highlight of the views.
				anchors.top: viewsHeader.bottom
				anchors.topMargin: Style.marginS
				anchors.left: parent.left
				anchors.right: parent.right

				height: Style.controlHeightM
				textFieldRightMargin: clearViewsFilterButton.width + Style.marginM + Style.marginXS
				placeHolderText: qsTr("Filter")
				radius: Style.textFieldRadius

				onTextChanged: {
					topologyPage.viewsFilterText = viewsFilterField.text
				}

				Button {
					id: clearViewsFilterButton

					anchors.verticalCenter: viewsFilterField.verticalCenter
					anchors.right: viewsFilterField.right
					anchors.rightMargin: Style.marginXS

					width: height
					height: 12
					z: viewsFilterField.z + 1

					decorator: Component { IconButtonDecorator {} }
					iconSource: "qrc:/" + Style.getIconPath("Icons/Close", Icon.State.On, Icon.Mode.Normal)

					visible: viewsFilterField.text !== ""
					enabled: visible

					onClicked: {
						viewsFilterField.text = ""
					}
				}
			}

			BaseText {
				anchors.top: viewsFlickable.top
				anchors.topMargin: Style.marginM
				anchors.left: parent.left
				anchors.leftMargin: Style.marginM
				anchors.right: parent.right
				anchors.rightMargin: Style.marginM

				visible: topologyPage.matchingViewsCount === 0
				text: qsTr("No views match the filter")
				color: Style.placeHolderTextColor
				font.pixelSize: Style.fontSizeS
				wrapMode: Text.WordWrap
			}

			// Views that do not fit the height scroll instead of being cut off.
			Flickable {
				id: viewsFlickable

				anchors.top: viewsFilterField.bottom
				anchors.topMargin: Style.marginS
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.bottom: parent.bottom

				contentWidth: width
				contentHeight: viewsColumn.height + 2 * Style.marginS
				boundsBehavior: Flickable.StopAtBounds
				interactive: contentHeight > height
				clip: true

				Column {
					id: viewsColumn

					y: Style.marginS
					width: viewsFlickable.width - (viewsScrollbar.visible ? viewsScrollbar.width + Style.marginXS : 0)

					spacing: Style.spacingXS

					Repeater {
						model: topologyPage.viewsModel

						delegate: SidePanelItem {
							id: viewButton

							objectName: model.pinned ? "TopologyViewAgentsButton" : "TopologyViewButton"

							width: viewsColumn.width
							// Views not matching the filter are hidden.
							visible: topologyPage.viewMatchesFilter(model.searchText, topologyPage.viewsFilterText)

							text: model.name
							icon: model.pinned ? "Icons/Agents" : "Icons/FolderClosed"
							selected: model.id === topologyPage.viewId

							Component.onCompleted: {
								viewButton.mouseArea.acceptedButtons = Qt.LeftButton | Qt.RightButton
							}

							// Left and right click select the view; the right click opens its context menu as well.
							onClicked: {
								topologyPage.selectView(model.id)
							}

							Connections {
								target: viewButton.mouseArea
								function onClicked(mouse) {
									if (mouse.button === Qt.RightButton){
										let point = viewButton.mouseArea.mapToItem(null, mouse.x, mouse.y)
										topologyPage.openViewContextMenu(model.id, point.x, point.y)
									}
								}
							}
						}
					}
				}
			}

			CustomScrollbar {
				id: viewsScrollbar

				anchors.right: parent.right
				anchors.top: viewsFlickable.top
				anchors.bottom: viewsFlickable.bottom

				secondSize: Style.marginS
				targetItem: viewsFlickable
			}
		}
	}

	// Navigation panel of the levels; shown inside a composite node only.
	StackViewHeader {
		id: stackHeader

		objectName: "TopologyNavigationPanel"

		anchors.top: parent.top
		anchors.left: viewsPanel.right
		anchors.leftMargin: Style.marginM
		anchors.right: metaInfo.left
		anchors.rightMargin: Style.marginM
		height: visible ? Style.controlHeightL : 0
		visible: schemeStack.count > 1
		initialItemTitleVisible: true

		onCloseClicked: {
			topologyPage.navigateToLevel(schemeStack.count - 2)
		}

		onHeaderItemClicked: {
			topologyPage.navigateToLevel(index)
		}

		Component.onCompleted: {
			stackHeader.addHeader("", qsTr("Agents"))
		}
	}

	StackView {
		id: schemeStack

		anchors.left: viewsPanel.right;
		anchors.right: metaInfo.left;
		anchors.top: stackHeader.bottom;
		anchors.bottom: parent.bottom;

		Component.onCompleted: {
			schemeStack.addPage(schemeLevelComp)
		}

		onPageAdded: {
			if (index >= 0 && index < topologyPage.levelNodeIds.length){
				item.groupId = topologyPage.levelNodeIds[index]
			}
			// The page may be loaded after it was made current.
			if (index === schemeStack.currentIndex){
				topologyPage.updateCurrentScheme()
				if (index > 0){
					topologyPage.topologyViewModel.loadTopology()
				}
			}
		}

		onCurrentPageChanged: {
			topologyPage.updateCurrentScheme()
		}
	}

	// One level of the navigation: the direct children of the composite node groupId.
	Component {
		id: schemeLevelComp

		SchemeView {
			id: levelScheme

			// Test instrumentation: SchemeView carries no objectName of its own, so the diagram would
			// otherwise be unaddressable. Inert.
			objectName: "TopologySchemeView"

			maximumObjectWidth: 360
			objectFontSize: Style.fontSizeXL
			objectSecondaryFontSize: Style.fontSizeS
			strokeSecondaryText: false

			// Composite node shown by the level, empty for the whole topology.
			property string groupId: ""
			// Selected node (service, agent or group); selectedService is set for services only.
			property string selectedNodeId: ""
			property string selectedService: ""
			// Models of the level, used in turn by renderLevel().
			property BaseModel firstModel: BaseModel {}
			property BaseModel secondModel: BaseModel {}

			onAutoFitChanged: {
				if (levelScheme === topologyPage.scheme && topologyPage.commandsController){
					topologyPage.commandsController.setToggled("AutoFit", levelScheme.autoFit);
				}
			}

			// The new positions go to the draft (saved with Save).
			onObjectMoveFinished: {
				topologyPage.storeLevelLayout(levelScheme)
			}

			// "Add to group" by drag and drop: a tile released over a blinking group is moved into it.
			// Inside the group it keeps its last position (the drag position is not taken).
			dropTargetFilter: function(objectId, targetId){
				return topologyPage.canAddToGroup(objectId, targetId)
			}

			// All selected tiles are dragged together.
			onObjectDropped: {
				let nodeIds = levelScheme.selectedIds.indexOf(objectId) >= 0 ? levelScheme.selectedIds.slice() : [objectId]
				for (let i = 0; i < nodeIds.length; ++i){
					if (!topologyPage.canAddToGroup(nodeIds[i], targetId)){
						return
					}
				}
				topologyPage.topologyViewModel.moveNodes(nodeIds, targetId)
			}

			onDeleteSignal: {
				topologyPage.requestDelete()
			}

			// F2; ignored while a dialog is open.
			onRenameSignal: {
				if (ModalDialogManager.count === 0){
					topologyPage.requestRename()
				}
			}

			// The tile is already selected by the right button press.
			onContextMenuRequested: {
				topologyPage.openContextMenu(menuX, menuY)
			}

			// selectedNodeId is the current node (also of several selected ones); selectedService is set
			// only when one service is selected (meta information, Start/Stop).
			onSelectionChanged: {
				let item = levelScheme.selectedIndex >= 0 && levelScheme.objectsModel
						&& levelScheme.selectedIndex < levelScheme.objectsModel.count
						? levelScheme.objectsModel.get(levelScheme.selectedIndex).item : null
				let isComposite = topologyPage.isCompositeNode(item)
				let single = levelScheme.selectedIds.length <= 1

				levelScheme.selectedNodeId = item ? item.m_id : ""
				levelScheme.selectedService = item && !isComposite && single ? item.m_id : ""

				topologyPage.applySelection(levelScheme)
			}

			// A click on a link shows the linked services in the panel on the right.
			onSelectedLinkChanged: {
				if (levelScheme === topologyPage.scheme){
					topologyPage.updateLinkInfo()
				}
			}

			// Double click / Return: goes inside a composite node, opens the editor of a service.
			function goInside(){
				let item = topologyPage.selectedNode()
				if (item === null){
					return
				}

				if (topologyPage.isCompositeNode(item)){
					topologyPage.openLevel(item.m_id, item.m_mainText)
				}
				else{
					topologyPage.openServiceDocument(item.m_id, item.m_mainText, item.m_agentId)
				}
			}

			function zoomToFit(){
				fitToCanvas()
			}

			// Disabled while any modal is open: otherwise Return is stolen from
			// FileSystemBrowserDialog (and any other dialog that needs Enter).
			// Only the shown level of the visible topology page reacts.
			Shortcut {
				sequence: "Return"
				enabled: ModalDialogManager.count === 0 && topologyPage.visible && levelScheme.visible
				onActivated: {
					levelScheme.goInside()
				}
			}
		}
	}

	// Status popup while Start/Stop is in progress (aligned with ServiceEditor.statusPopup).
	// Width is derived from available space + text implicitWidth only — no circular bindings.
	Rectangle {
		id: serviceOperationIndicator
		property int spinnerSize: 18
		property real maxPopupWidth: Math.max(0, metaInfo.x - viewsPanel.width - 2 * Style.marginXL)
		property real maxTextWidth: Math.max(0, maxPopupWidth - 2 * Style.marginL - spinnerSize - Style.spacingS)
		property real textWidth: Math.min(serviceOperationTextItem.implicitWidth, maxTextWidth)

		anchors.top: schemeStack.top
		anchors.right: metaInfo.left
		anchors.topMargin: Style.marginXL
		anchors.rightMargin: Style.marginXL
		width: Math.min(
			spinnerSize + Style.spacingS + textWidth + 2 * Style.marginL,
			maxPopupWidth)
		height: 36
		clip: true
		radius: height / 2
		color: Style.backgroundColor
		border.color: Style.lightBlueColor
		border.width: 1
		visible: topologyPage.serviceOperationInProgress && topologyPage.serviceOperationText !== ""
		z: 100

		Row {
			id: serviceOperationRow
			anchors.centerIn: parent
			spacing: Style.spacingS

			Loading {
				anchors.verticalCenter: parent.verticalCenter
				width: serviceOperationIndicator.spinnerSize
				height: serviceOperationIndicator.spinnerSize
				indicatorSize: 14
				background.color: "transparent"
			}

			BaseText {
				id: serviceOperationTextItem
				anchors.verticalCenter: parent.verticalCenter
				width: serviceOperationIndicator.textWidth
				text: topologyPage.serviceOperationText
				font.pixelSize: Style.fontSizeM
				elide: Text.ElideRight
			}
		}
	}

	// Information on the clicked link (in place of the meta information of a service, over it): which
	// service is linked with which one. A click on a service name opens the page of the service.
	Rectangle {
		id: linkInfoPanel

		objectName: "TopologyLinkInfoPanel"

		anchors.fill: metaInfo
		z: metaInfo.z + 1

		visible: topologyPage.linkInfo !== null
		clip: true
		// The same background as the meta information panel
		color: linkInfoDecorator.item && linkInfoDecorator.item.color !== undefined ? linkInfoDecorator.item.color : Style.baseColor

		Loader {
			id: linkInfoDecorator

			anchors.fill: parent
			sourceComponent: Style.metaInfoPanelDecorator
		}

		Flickable {
			id: linkInfoFlickable

			anchors.fill: parent

			contentWidth: width
			contentHeight: linkInfoColumn.height + 2 * Style.marginM
			boundsBehavior: Flickable.StopAtBounds

			Column {
				id: linkInfoColumn

				anchors.top: parent.top
				anchors.topMargin: Style.marginM
				anchors.left: parent.left
				anchors.leftMargin: Style.marginM
				anchors.right: parent.right
				anchors.rightMargin: Style.marginM + linkInfoScrollbar.secondSize

				spacing: Style.spacingL

				Column {
					width: linkInfoColumn.width
					spacing: Style.spacingM

					BaseText {
						width: parent.width
						text: qsTr("Link")
						font.pixelSize: Style.fontSizeM
						font.family: Style.fontFamilyBold
						font.bold: true
						elide: Text.ElideRight
					}

					BaseText {
						width: parent.width
						text: topologyPage.linkInfo ? topologyPage.linkInfo.fromName + " — " + topologyPage.linkInfo.toName : ""
						color: Style.subtitleColor
						font.pixelSize: Style.fontSizeS
						wrapMode: Text.WordWrap
					}
				}

				Repeater {
					model: topologyPage.linkInfo ? topologyPage.linkInfo.pairs : []

					delegate: Column {
						id: linkPairColumn

						width: linkInfoColumn.width
						spacing: Style.spacingXS

						property var pair: modelData

						Repeater {
							model: [
								{"service": linkPairColumn.pair.source, "caption": ""},
								{"service": linkPairColumn.pair.target, "caption": qsTr("is linked with")}
							]

							delegate: Column {
								width: linkPairColumn.width
								spacing: Style.spacingXS

								BaseText {
									width: parent.width
									visible: modelData.caption !== ""
									text: "↓ " + modelData.caption
									color: Style.subtitleColor
									font.pixelSize: Style.fontSizeS
								}

								Text {
									id: serviceNameText

									width: parent.width
									text: modelData.service.name
									color: Style.lightBlueColor
									font.family: Style.fontFamily
									font.pixelSize: Style.fontSizeM
									font.underline: serviceNameMouseArea.containsMouse
									wrapMode: Text.WrapAnywhere

									MouseArea {
										id: serviceNameMouseArea

										anchors.fill: parent
										hoverEnabled: true
										cursorShape: Qt.PointingHandCursor
										onClicked: {
											topologyPage.openServiceDocument(modelData.service.id, modelData.service.name, modelData.service.agentId)
										}
									}
								}

								BaseText {
									width: parent.width
									visible: modelData.service.path !== ""
									text: modelData.service.path
									color: Style.subtitleColor
									font.pixelSize: Style.fontSizeS
									elide: Text.ElideRight
								}
							}
						}

						// Separator between the pairs
						Item {
							width: parent.width
							height: Style.spacingM
						}

						Rectangle {
							width: parent.width
							height: 1
							color: Style.borderColor
							opacity: 0.5
						}
					}
				}
			}
		}

		CustomScrollbar {
			id: linkInfoScrollbar

			z: linkInfoFlickable.z + 1
			anchors.right: parent.right
			anchors.top: linkInfoFlickable.top
			anchors.bottom: linkInfoFlickable.bottom
			secondSize: Style.marginS
			targetItem: linkInfoFlickable
			alwaysVisible: false
		}
	}

	MetaInfo {
		id: metaInfo;
		
		anchors.right: parent.right;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;

		// Always shown with the same width, so that the size of the scheme does not change with the
		// selection; its content is shown for a selected service only.
		width: 250;
		// Nothing is selected at first.
		contentVisible: false;

		Component.onCompleted: {
			registerViewDelegate("ExternPaths", metaInfoViewExternPathsDelegateComp)
		}

		BaseText {
			anchors.top: parent.top
			anchors.topMargin: Style.marginM
			anchors.left: parent.left
			anchors.leftMargin: Style.marginM
			anchors.right: parent.right
			anchors.rightMargin: Style.marginM

			visible: !metaInfo.contentVisible && topologyPage.linkInfo === null
			text: qsTr("Select a service or a link to see its information")
			color: Style.placeHolderTextColor
			font.pixelSize: Style.fontSizeS
			wrapMode: Text.WordWrap
		}
	}
	
	CollectionRepresentation {
		id: collectionDataProvider
		collectionId: "Services"
		onObjectMetaInfoReceived: {
			metaInfo.elementMetaInfo = metaInfoData
		}
		onObjectMetaInfoReceiveFailed: {
			// Deleted / missing service — keep the panel quiet rather than showing a stale error.
			if (topologyPage.selectedService === "" || !topologyPage.scheme || topologyPage.scheme.findModelIndex(topologyPage.selectedService) < 0){
				metaInfo.contentVisible = false
			}
		}
		// RemoveElements success: do not wait for OnServicesCollectionChanged (can lag or
		// miss). Clear selection so nothing re-requests meta for a deleted id, then reload.
		onElementsRemoved: {
			let removedIds = elementIds
			let selectedGone = false
			if (removedIds && topologyPage.selectedService !== ""){
				for (let i = 0; i < removedIds.length; i++){
					if (("" + removedIds[i]) === topologyPage.selectedService){
						selectedGone = true
						break
					}
				}
			}
			if (selectedGone)
				topologyPage.clearServiceSelection()
			topologyPage.topologyViewModel.requestReload()
		}
		
		function getHeaders(){
			return topologyPage.getHeaders();
		}
	}
	
	function getHeaders(){
		// Thin adapter for ImtCore widgets — scope lives on topologyViewModel.dataScope.
		if (topologyViewModel.dataScope) {
			topologyViewModel.dataScope.agentId = topologyPage.selectedAgentId
			topologyViewModel.dataScope.serviceId = topologyPage.selectedService
		}
		let headers = {}
		if (topologyPage.selectedAgentId !== ""){
			headers["clientid"] = topologyPage.selectedAgentId;
		}
		if (topologyPage.selectedService !== ""){
			headers["serviceid"] = topologyPage.selectedService;
		}
		return headers
	}

	Component {
		id: metaInfoViewExternPathsDelegateComp
		MetaInfoViewDelegateBase {
			id: metaInfoDelegate

			height: pathColumn.height

			// Server may send "No Extern Paths" (or similar) when the list is empty —
			// that must not render as a clickable URL.
			function isOpenableUrl(value){
				if (!value || value === "")
					return false
				return value.indexOf("://") >= 0
			}

			Column {
				id: pathColumn
				width: metaInfoDelegate.width
				spacing: Style.spacingS
				Repeater {
					model: metaInfoDelegate.viewData.split('\n')
					delegate: Item {
						height: linkText.height
						width: parent.width
						readonly property bool isLink: metaInfoDelegate.isOpenableUrl(modelData)
						Text {
							id: linkText
							width: parent.width
							font.family: Style.fontFamily
							font.pixelSize: Style.fontSizeS
							font.underline: parent.isLink
							font.italic: !parent.isLink
							wrapMode: Text.WordWrap
							color: parent.isLink ? Style.lightBlueColor : Style.subtitleColor
							elide: Text.ElideRight
							text: modelData
						}

						MouseArea {
							anchors.fill: linkText
							hoverEnabled: parent.isLink
							enabled: parent.isLink
							cursorShape: parent.isLink ? Qt.PointingHandCursor : Qt.ArrowCursor
							onClicked: {
								if (!parent.isLink)
									return
								Qt.openUrlExternally(modelData)
							}
						}
					}
				}
			}
		}
	}
	
	Component {
		id: serviceValidatorComp;
		
		ServiceValidator {}
	}

	Component {
		id: viewContextMenuComp

		PopupMenuDialog {
			// View the menu was opened for.
			property string contextViewId: ""

			onFinished: {
				if (commandId === "RemoveView"){
					topologyPage.requestRemoveView(contextViewId)
				}
			}
		}
	}

	Component {
		id: newViewDialogComp

		InputDialog {
			title: qsTr("New view")
			message: qsTr("View name")
			placeHolderText: qsTr("Enter the view name")
			onFinished: {
				let name = inputValue ? inputValue.trim() : ""
				if (buttonId == Enums.ok && name !== ""){
					topologyPage.topologyViewModel.createView(name)
				}
			}
		}
	}

	// Asked before another view is shown while the current one has unsaved changes:
	// Save (then continue), Discard (then continue) or Cancel.
	Component {
		id: unsavedChangesDialogComp

		Dialog {
			id: unsavedChangesDialog

			width: Style.sizeHintM
			title: qsTr("Unsaved changes")
			canMove: true
			forceFocus: true

			property string viewName: ""
			// Called after the changes were saved or discarded.
			property var continueAction: null

			Component.onCompleted: {
				addButton(Enums.yes, qsTr("Save"), true, true)
				addButton(Enums.no, qsTr("Discard"), true)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			onFinished: {
				let action = unsavedChangesDialog.continueAction
				if (!action){
					return
				}
				if (buttonId === Enums.yes){
					// A failed save keeps the view and its draft (the error is shown).
					topologyPage.topologyViewModel.save(function(ok){
						if (ok){
							action()
						}
					})
				}
				else if (buttonId === Enums.no){
					topologyPage.topologyViewModel.discardChanges()
					action()
				}
			}

			contentComp: Component {
				MessageDialogBody {
					width: unsavedChangesDialog.width
					message: qsTr("The view \"%1\" has unsaved changes. Save them?").arg(unsavedChangesDialog.viewName)
				}
			}
		}
	}

	Component {
		id: removeViewDialogComp

		MessageDialog {
			property string viewId: ""
			property string viewName: ""

			width: Style.sizeHintM
			title: qsTr("Deleting a view")
			message: qsTr("Delete the view \"%1\" with all its groups? The services are not affected.").arg(viewName)
			onFinished: {
				if (buttonId == Enums.yes && viewId !== ""){
					topologyPage.topologyViewModel.removeView(viewId)
				}
			}
		}
	}

	Component {
		id: topologyContextMenuComp

		PopupMenuDialog {
			onFinished: {
				if (commandId !== "" && topologyPage.commandsDelegate){
					topologyPage.commandsDelegate.commandHandle(commandId)
				}
			}
		}
	}

	Component {
		id: removeServiceDialogComp

		// Physical deletion of the service: emphasised as a dangerous action. Cancel is the default
		// button, so Enter does not delete the service by accident.
		Dialog {
			id: removeServiceDialog

			width: Style.sizeHintM
			title: qsTr("Delete service permanently")
			canMove: true
			forceFocus: true

			property string serviceId: ""
			property string serviceName: ""
			property string agentName: ""

			Component.onCompleted: {
				addButton(Enums.yes, qsTr("Delete service"), true, false, "danger")
				addButton(Enums.cancel, qsTr("Cancel"), true, true)
			}

			onFinished: {
				// The selection may have changed while the dialog was open.
				if (buttonId !== Enums.yes || removeServiceDialog.serviceId === ""
						|| removeServiceDialog.serviceId !== topologyPage.selectedService){
					return
				}
				if (!topologyPage.canRemoveSelectedService()){
					topologyPage.showCannotRemoveWhileDisconnected()
					return
				}
				collectionDataProvider.removeElements([removeServiceDialog.serviceId])
			}

			contentComp: Component {
				Item {
					width: removeServiceDialog.width
					height: dangerColumn.height + 2 * Style.marginL

					Column {
						id: dangerColumn

						anchors.top: parent.top
						anchors.topMargin: Style.marginL
						anchors.left: parent.left
						anchors.leftMargin: Style.marginL
						anchors.right: parent.right
						anchors.rightMargin: Style.marginL

						spacing: Style.marginM

						// Warning banner
						Rectangle {
							width: dangerColumn.width
							height: bannerText.height + 2 * Style.marginM

							color: Style.dangerSubtleColor
							border.width: 1
							border.color: Style.dangerColor
							radius: Style.radiusM

							Image {
								id: bannerIcon

								anchors.left: parent.left
								anchors.leftMargin: Style.marginM
								anchors.verticalCenter: parent.verticalCenter

								width: Style.iconSizeM
								height: width
								sourceSize.width: width
								sourceSize.height: height
								source: "qrc:/" + Style.getIconPath("Icons/Warning", Icon.State.On, Icon.Mode.Normal)
							}

							BaseText {
								id: bannerText

								anchors.left: bannerIcon.right
								anchors.leftMargin: Style.marginM
								anchors.right: parent.right
								anchors.rightMargin: Style.marginM
								anchors.verticalCenter: parent.verticalCenter

								text: qsTr("The service will be deleted from the system, not only from this tile. This action cannot be undone.")
								color: Style.dangerColor
								font.family: Style.fontFamilyBold
								font.bold: true
								wrapMode: Text.WordWrap
							}
						}

						BaseText {
							width: dangerColumn.width

							text: removeServiceDialog.agentName !== ""
								? qsTr("Service \"%1\" on agent \"%2\"").arg(removeServiceDialog.serviceName).arg(removeServiceDialog.agentName)
								: qsTr("Service \"%1\"").arg(removeServiceDialog.serviceName)
							font.family: Style.fontFamilyBold
							font.bold: true
							wrapMode: Text.WordWrap
						}

						BaseText {
							width: dangerColumn.width

							text: qsTr("Consequences:") + "\n"
								+ "•  " + qsTr("a running service is stopped, then deleted on its agent together with its settings;") + "\n"
								+ "•  " + qsTr("it disappears from all views and groups of the topology;") + "\n"
								+ "•  " + qsTr("services that depend on it may stop working;") + "\n"
								+ "•  " + qsTr("Undo cannot restore it: the service has to be created and configured again.")
							wrapMode: Text.WordWrap
						}

						BaseText {
							width: dangerColumn.width

							text: qsTr("To remove the service only from a view, use Remove in that view instead.")
							color: Style.subtitleColor
							font.pixelSize: Style.fontSizeS
							wrapMode: Text.WordWrap
						}
					}
				}
			}
		}
	}

	Component {
		id: removeGroupDialogComp

		MessageDialog {
			property string groupId: ""

			width: Style.sizeHintM
			title: qsTr("Deleting a group")
			message: qsTr("Delete the selected group? Its content will be moved one level up.")
			onFinished: {
				if (buttonId == Enums.yes && groupId !== ""){
					topologyPage.topologyViewModel.removeGroup(groupId)
				}
			}
		}
	}

	Component {
		id: removeFromViewDialogComp

		MessageDialog {
			property string nodeId: ""

			width: Style.sizeHintM
			title: qsTr("Removing from the view")
			message: qsTr("Remove the selected service from the view \"%1\"? The service itself is not deleted.")
						.arg(topologyPage.getViewName(topologyPage.viewId))
			onFinished: {
				if (buttonId == Enums.yes && nodeId !== ""){
					topologyPage.topologyViewModel.removeFromView([nodeId])
				}
			}
		}
	}

	Component {
		id: renameDialogComp

		InputDialog {
			property string nodeId: ""
			property string agentId: ""
			property bool isGroup: false
			property string oldName: ""

			message: qsTr("Name")
			placeHolderText: qsTr("Enter the name")
			selectTextOnStart: true

			onFinished: {
				let name = inputValue ? inputValue.trim() : ""
				if (buttonId != Enums.ok || name === "" || name === oldName || nodeId === ""){
					return
				}
				// A group is renamed in the draft; a service is renamed on its agent at once (the name
				// belongs to the service, not to the view).
				if (isGroup){
					topologyPage.topologyViewModel.renameGroup(nodeId, name)
				}
				else{
					topologyPage.topologyViewModel.renameService(nodeId, agentId, name)
				}
			}
		}
	}

	Component {
		id: newGroupDialogComp

		InputDialog {
			// Nodes moved into the new group ("Add to group" without a group to add them to); empty for "New group".
			property var nodeIds: []

			title: qsTr("New group")
			message: nodeIds.length > 0 ? qsTr("There is no group to add the selected items to. Name of the new group for them")
										: qsTr("Group name")
			placeHolderText: qsTr("Enter the group name")
			onFinished: {
				let name = inputValue ? inputValue.trim() : ""
				if (buttonId != Enums.ok || name === ""){
					return
				}
				// Groups live at the root level or inside other groups (not inside an agent).
				let parentId = topologyPage.isGroupNode(topologyPage.groupId) ? topologyPage.groupId : ""
				if (nodeIds.length > 0){
					let position = topologyPage.findGroupPosition(nodeIds)
					topologyPage.topologyViewModel.createGroupWithNodes(name, parentId, position.x, position.y, nodeIds)
				}
				else{
					let position = topologyPage.findFreePosition(topologyPage.scheme ? topologyPage.scheme.objectsModel : null)
					topologyPage.topologyViewModel.createGroup(name, parentId, position.x, position.y)
				}
			}
		}
	}

	Component {
		id: addToViewDialogComp

		Dialog {
			id: addToViewDialog
			width: Style.sizeHintM
			title: qsTr("Add services to view")

			property string parentId: ""
			// [{id, name}] of the services that are not in the view yet.
			property var services: []
			property var selectedIds: []
			property string filterText: ""
			// Services whose name contains the filter text (case-insensitive); the selection is kept while filtering.
			readonly property var filteredServices: {
				let filter = addToViewDialog.filterText.trim().toLowerCase()
				if (filter === "")
					return addToViewDialog.services
				let result = []
				for (let i = 0; i < addToViewDialog.services.length; ++i){
					if (addToViewDialog.services[i].name.toLowerCase().indexOf(filter) >= 0){
						result.push(addToViewDialog.services[i])
					}
				}
				return result
			}

			// Dialog parameters are assigned after the dialog is created.
			onServicesChanged: {
				addToViewDialog.selectedIds = []
				addToViewDialog.setButtonEnabled(Enums.apply, false)
			}

			function toggleService(serviceId){
				let ids = addToViewDialog.selectedIds.slice()
				let index = ids.indexOf(serviceId)
				if (index >= 0){
					ids.splice(index, 1)
				}
				else{
					ids.push(serviceId)
				}
				addToViewDialog.selectedIds = ids
				addToViewDialog.setButtonEnabled(Enums.apply, ids.length > 0)
			}

			Component.onCompleted: {
				addButton(Enums.apply, qsTr("Add"), false)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			onFinished: {
				if (buttonId === Enums.apply && addToViewDialog.selectedIds.length > 0){
					topologyPage.topologyViewModel.addToView(addToViewDialog.selectedIds, addToViewDialog.parentId)
				}
			}

			contentComp: Component {
				Item {
					width: addToViewDialog.width
					height: messageText.height + filterField.height + servicesFrame.height + selectedText.height
							+ 3 * Style.marginM + 2 * Style.marginL

					BaseText {
						id: messageText
						anchors.top: parent.top
						anchors.topMargin: Style.marginL
						anchors.left: parent.left
						anchors.leftMargin: Style.marginL
						anchors.right: parent.right
						anchors.rightMargin: Style.marginL
						wrapMode: Text.WordWrap
						text: addToViewDialog.parentId !== ""
							? qsTr("Select the services to add to the group \"%1\"").arg(topologyPage.getNodePath(addToViewDialog.parentId))
							: qsTr("Select the services to add to the view")
					}

					CustomTextField {
						id: filterField
						objectName: "TopologyAddToViewFilter"
						anchors.top: messageText.bottom
						anchors.topMargin: Style.marginM
						anchors.left: parent.left
						anchors.leftMargin: Style.marginL
						anchors.right: parent.right
						anchors.rightMargin: Style.marginL
						height: Style.controlHeightM
						textFieldRightMargin: clearFilterButton.width + Style.marginM + Style.marginXS
						placeHolderText: qsTr("Filter by name")
						radius: Style.textFieldRadius

						onTextChanged: {
							addToViewDialog.filterText = filterField.text
						}

						Component.onCompleted: {
							filterField.forceInputFocus()
						}

						Button {
							id: clearFilterButton
							anchors.verticalCenter: filterField.verticalCenter
							anchors.right: filterField.right
							anchors.rightMargin: Style.marginXS
							width: height
							height: 12
							z: filterField.z + 1
							decorator: Component { IconButtonDecorator {} }
							iconSource: "qrc:/" + Style.getIconPath("Icons/Close", Icon.State.On, Icon.Mode.Normal)
							visible: filterField.text !== ""
							enabled: visible
							onClicked: {
								filterField.text = ""
							}
						}
					}

					// Fixed height: the dialog does not jump while filtering; long lists are scrolled.
					Rectangle {
						id: servicesFrame
						anchors.top: filterField.bottom
						anchors.topMargin: Style.marginM
						anchors.left: parent.left
						anchors.leftMargin: Style.marginL
						anchors.right: parent.right
						anchors.rightMargin: Style.marginL
						height: 8 * Style.controlHeightL + 2
						color: "transparent"
						border.width: 1
						border.color: Style.borderColor
						radius: Style.radiusS

						BaseText {
							anchors.centerIn: parent
							visible: servicesList.count === 0
							color: Style.placeHolderTextColor
							text: addToViewDialog.services.length === 0
								? qsTr("All services are already in the view")
								: qsTr("No services match the filter")
						}

						ListView {
							id: servicesList
							anchors.fill: parent
							anchors.margins: 1
							anchors.rightMargin: Style.marginS + 2

							clip: true
							boundsBehavior: Flickable.StopAtBounds
							model: addToViewDialog.filteredServices

							delegate: Rectangle {
								id: serviceRow
								width: servicesList.width
								height: Style.controlHeightL
								color: serviceRowMouseArea.containsMouse ? Style.hover : "transparent"

								property bool selected: addToViewDialog.selectedIds.indexOf(modelData.id) >= 0

								MouseArea {
									id: serviceRowMouseArea
									anchors.fill: parent
									hoverEnabled: true
									cursorShape: Qt.PointingHandCursor
									onClicked: {
										addToViewDialog.toggleService(modelData.id)
									}
								}

								CheckBox {
									id: serviceCheckBox
									anchors.left: parent.left
									anchors.leftMargin: Style.marginM
									anchors.verticalCenter: parent.verticalCenter
									isActive: false
									checkState: serviceRow.selected ? Qt.Checked : Qt.Unchecked
								}

								BaseText {
									anchors.left: serviceCheckBox.right
									anchors.leftMargin: Style.marginM
									anchors.right: parent.right
									anchors.rightMargin: Style.marginM
									anchors.verticalCenter: parent.verticalCenter
									text: modelData.name
									elide: Text.ElideRight
								}
							}
						}

						CustomScrollbar {
							z: servicesList.z + 1
							anchors.right: parent.right
							anchors.rightMargin: 1
							anchors.top: servicesList.top
							anchors.bottom: servicesList.bottom
							secondSize: Style.marginS
							radius: Style.radiusS
							targetItem: servicesList
						}
					}

					BaseText {
						id: selectedText
						anchors.top: servicesFrame.bottom
						anchors.topMargin: Style.marginM
						anchors.left: parent.left
						anchors.leftMargin: Style.marginL
						color: Style.placeHolderTextColor
						font.pixelSize: Style.fontSizeS
						text: qsTr("Selected: %1 of %2").arg(addToViewDialog.selectedIds.length).arg(addToViewDialog.services.length)
					}
				}
			}
		}
	}
	Component {
		id: addToGroupDialogComp

		Dialog {
			id: addToGroupDialog
			width: Style.sizeHintM
			title: qsTr("Add to group")

			// Nodes moved into the chosen group.
			property var nodeIds: []
			property var targetGroups: []
			property string chosenGroupId: ""

			Component.onCompleted: {
				addButton(Enums.apply, qsTr("Add"), false)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			onFinished: {
				if (buttonId === Enums.apply && nodeIds.length > 0 && chosenGroupId !== ""){
					topologyPage.topologyViewModel.moveNodes(nodeIds, chosenGroupId)
				}
			}

			contentComp: Component {
				Item {
					width: addToGroupDialog.width
					height: Style.controlHeightM + 2 * Style.marginL

					ComboBox {
						id: groupCombo
						anchors.centerIn: parent
						width: parent.width - 2 * Style.marginL
						height: Style.controlHeightM

						// Dialog parameters are assigned after the dialog is created.
						property var groups: addToGroupDialog.targetGroups

						model: TreeItemModel {}

						function fillGroups(){
							groupCombo.currentIndex = -1
							groupCombo.model.clear()
							for (let i = 0; i < groupCombo.groups.length; ++i){
								let index = groupCombo.model.insertNewItem()
								groupCombo.model.setData("id", groupCombo.groups[i].id, index)
								groupCombo.model.setData("name", groupCombo.groups[i].name, index)
							}
							if (groupCombo.groups.length > 0){
								groupCombo.currentIndex = 0
							}
						}

						onGroupsChanged: fillGroups()
						Component.onCompleted: fillGroups()

						onCurrentIndexChanged: {
							if (currentIndex >= 0 && currentIndex < groupCombo.groups.length){
								addToGroupDialog.chosenGroupId = groupCombo.groups[currentIndex].id
								addToGroupDialog.setButtonEnabled(Enums.apply, true)
							}
							else{
								addToGroupDialog.chosenGroupId = ""
								addToGroupDialog.setButtonEnabled(Enums.apply, false)
							}
						}
					}
				}
			}
		}
	}
	
	Component {
		id: serviceEditorComp;
		
		ServiceEditorWrap {
			id: serviceEditor
			documentManager: topologyPage.documentManager
			// Binding (not only onCompleted) so clientid header stays current for FS browse.
			clientId: topologyPage.documentAgentId
		}
	}
	
	Component {
		id: serviceDataControllerComp
		
		ServiceDocumentDataController {
			property string clientId: ""

			Component.onCompleted: {
				clientId = topologyPage.documentAgentId
			}

			function getHeaders(){
				let headers = {}
				if (clientId !== ""){
					headers["clientid"] = clientId
				}
				return headers
			}
		}
	}
	
	// Transport (subscriptions + Get/SaveTopology) lives in topologyViewModel.
}
