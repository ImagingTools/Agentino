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
		}
		topologyPage.selectedAgentId = ""
		metaInfo.contentVisible = false
		topologyPage.updateCommandStates()
	}

	function selectedNode(){
		let currentScheme = topologyPage.scheme
		if (!currentScheme || currentScheme.selectedIndex < 0 || !currentScheme.objectsModel
				|| currentScheme.selectedIndex >= currentScheme.objectsModel.count)
			return null
		let item = currentScheme.objectsModel.get(currentScheme.selectedIndex).item
		return item ? item : null
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

	// A missing model is an incomplete response, not a proof that the node was removed.
	function modelContainsNode(nodesModel, nodeId){
		if (!nodesModel)
			return true
		for (let i = 0; i < nodesModel.count; ++i){
			let item = nodesModel.get(i).item
			if (item && item.m_id === nodeId)
				return true
		}
		return false
	}

	// Remembers all nodes of the loaded topology and keeps on the scheme only the children of groupId.
	function applyTopologyNodes(nodesModel){
		let nodes = {}
		if (!nodesModel){
			topologyPage.topologyNodes = nodes
			return nodesModel
		}

		for (let i = 0; i < nodesModel.count; ++i){
			let item = nodesModel.get(i).item
			if (!item)
				continue
			nodes[item.m_id] = {
				"id": item.m_id,
				"parentId": item.m_parentId ? item.m_parentId : "",
				"nodeType": item.m_nodeType ? item.m_nodeType : "SERVICE",
				"name": item.m_mainText,
				"agentId": item.m_agentId
			}
		}
		topologyPage.topologyNodes = nodes

		for (let i = nodesModel.count - 1; i >= 0; --i){
			let item = nodesModel.get(i).item
			let parentId = item && item.m_parentId ? item.m_parentId : ""
			if (!item || parentId !== topologyPage.groupId){
				nodesModel.removeElement(i)
			}
		}

		// E.g. agents shown as composite nodes for the first time; the position is stored with the next layout change.
		for (let i = 0; i < nodesModel.count; ++i){
			let item = nodesModel.get(i).item
			if (item.m_x === 0 && item.m_y === 0){
				let position = topologyPage.findFreePosition(nodesModel)
				item.m_x = position.x
				item.m_y = position.y
			}
		}

		return nodesModel
	}

	// Inactive commands are hidden in the toolbar (and are not offered in the context menu).
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
		topologyPage.setCommandState("Edit", item !== null)
		// Delete requires a live agent — mirror-only delete comes back on reconnect. Agents cannot be removed here.
		topologyPage.setCommandState("Remove", isComposite ? topologyPage.isGroupNode(item.m_id) : serviceSelected && agentOnline)
		// Groups live at the root level or inside other groups; an agent contains only its services.
		topologyPage.setCommandState("NewGroup", topologyPage.groupId === "" || topologyPage.isGroupNode(topologyPage.groupId))
		topologyPage.setCommandState("AddToGroup", item !== null && topologyPage.getTargetGroups(item.m_id).length > 0)
		topologyPage.setCommandState("RemoveFromGroup", item !== null && topologyPage.isGroupNode(topologyPage.groupId))
	}

	property TreeItemModel contextMenuModel: TreeItemModel {}

	// Context menu of the selected tile: the active commands of the toolbar, handled by the same delegate.
	function openContextMenu(x, y){
		let commandsController = topologyPage.commandsController
		if (!commandsController || !topologyPage.commandsDelegate)
			return

		let commands = [
			{"id": "Remove", "name": qsTr("Remove"), "icon": "Icons/Delete"},
			{"id": "Edit", "name": qsTr("Edit"), "icon": "Icons/Edit"},
			{"id": "AddToGroup", "name": qsTr("Add to group"), "icon": "Icons/Link"},
			{"id": "RemoveFromGroup", "name": qsTr("Remove from group"), "icon": "Icons/Unlink"},
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
		topologyPage.applySelection(currentScheme)
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

	// Index of the first level whose node no longer exists (removed by this or another client), -1 if all exist.
	function findRemovedLevel(nodesModel){
		for (let i = 1; i < topologyPage.levelNodeIds.length; ++i){
			if (!topologyPage.modelContainsNode(nodesModel, topologyPage.levelNodeIds[i]))
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

	function requestAddToGroup(){
		let item = topologyPage.selectedNode()
		if (!item)
			return
		ModalDialogManager.openDialog(addToGroupDialogComp, {"nodeId": item.m_id, "targetGroups": topologyPage.getTargetGroups(item.m_id)})
	}

	// Moves the node one level up: into the group containing the current group, or back to its default place.
	function requestRemoveFromGroup(){
		let item = topologyPage.selectedNode()
		if (!item || !topologyPage.isGroupNode(topologyPage.groupId))
			return
		let currentGroup = topologyPage.topologyNodes[topologyPage.groupId]
		let targetGroupId = topologyPage.isGroupNode(currentGroup.parentId) ? currentGroup.parentId : ""
		topologyPage.topologyViewModel.moveNodes([item.m_id], targetGroupId)
	}

	function requestRemoveSelected(){
		let item = topologyPage.selectedNode()
		if (!item)
			return
		if (topologyPage.isCompositeNode(item)){
			if (topologyPage.isGroupNode(item.m_id)){
				ModalDialogManager.openDialog(removeGroupDialogComp, {"groupId": item.m_id})
			}
			return
		}
		if (!topologyPage.canRemoveSelectedService()){
			topologyPage.showCannotRemoveWhileDisconnected()
			return
		}
		ModalDialogManager.openDialog(removeServiceDialogComp, {})
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
	}

	Connections {
		target: topologyPage.topologyViewModel
		// Signal parameters must be listed in function onXxx(...) — without them
		// the body reads outer/undefined names (data/topologySdlObject) and crashes.
		function onTopologyLoaded(topologySdlObject) {
			if (!topologySdlObject){
				return
			}
			// A shown composite node was removed (here or by another client): return to its parent level.
			let nodesModel = topologySdlObject.m_services
			let removedLevel = topologyPage.findRemovedLevel(nodesModel)
			if (removedLevel > 0){
				topologyPage.navigateToLevel(removedLevel - 1)
				return
			}
			let currentScheme = topologyPage.scheme
			if (!currentScheme){
				return
			}
			// Re-find selection after full reload (see prior comments on selectedAgentId).
			let previousNodeId = currentScheme.selectedNodeId
			currentScheme.selectedIndex = -1
			currentScheme.objectsModel = topologyPage.applyTopologyNodes(nodesModel)
			currentScheme.selectedIndex = currentScheme.findModelIndex(previousNodeId)
			topologyPage.updateLevelNames()
			topologyPage.updateCommandStates()
			currentScheme.requestPaint()
		}
		function onServiceStatusMessage(data) {
			topologyPage.applyLiveServiceStatus(data)
		}
		function onGroupOperationFailed(message) {
			ModalDialogManager.showErrorDialog(message !== "" ? message : qsTr("Unable to change the topology groups"))
		}
	}
	
	commandsControllerComp: Component {GqlBasedCommandsController {
			typeId: "Topology"
			
			onCommandsChanged: {
				setCommandVisible("Save", false)
				setCommandIsEnabled("New", true)
				setIsToggleable("AutoFit", true);
				setToggled("AutoFit", topologyPage.scheme ? topologyPage.scheme.autoFit : true);
				topologyPage.updateCommandStates()
			}
		}
	}
	
	onVisibleChanged: {
		if (topologyPage.visible){
			topologyViewModel.loadTopology()
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
					else if (commandId === "Edit"){
						onEdit(params)
					}
					else if (commandId === "Remove"){
						onRemove(params)
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
			
			function onEdit(){
				if (topologyPage.scheme){
					topologyPage.scheme.goInside()
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
		if (!levelScheme || levelScheme !== topologyPage.scheme){
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

	// Navigation panel of the levels; shown inside a composite node only.
	StackViewHeader {
		id: stackHeader

		objectName: "TopologyNavigationPanel"

		anchors.top: parent.top
		anchors.left: parent.left
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
			stackHeader.addHeader("", qsTr("Topology"))
		}
	}

	StackView {
		id: schemeStack

		anchors.left: parent.left;
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

			onAutoFitChanged: {
				if (levelScheme === topologyPage.scheme && topologyPage.commandsController){
					topologyPage.commandsController.setToggled("AutoFit", levelScheme.autoFit);
				}
			}

			onObjectMoveFinished: {
				// saveTopology() sets saveInProgress so OnTopologyChanged does not reload mid-save.
				topologyPage.topologyViewModel.saveTopology(levelScheme.objectsModel)
			}

			onDeleteSignal: {
				topologyPage.requestRemoveSelected()
			}

			// The tile is already selected by the right button press.
			onContextMenuRequested: {
				topologyPage.openContextMenu(menuX, menuY)
			}

			onSelectedIndexChanged: {
				let item = levelScheme.selectedIndex >= 0 && levelScheme.objectsModel
						&& levelScheme.selectedIndex < levelScheme.objectsModel.count
						? levelScheme.objectsModel.get(levelScheme.selectedIndex).item : null
				let isComposite = topologyPage.isCompositeNode(item)

				levelScheme.selectedNodeId = item ? item.m_id : ""
				levelScheme.selectedService = item && !isComposite ? item.m_id : ""

				topologyPage.applySelection(levelScheme)
			}

			// Double click / Return / Edit: goes inside a composite node, opens the editor of a service.
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
		property real maxPopupWidth: Math.max(0, metaInfo.x - 2 * Style.marginXL)
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

	MetaInfo {
		id: metaInfo;
		
		anchors.right: parent.right;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;

		// Takes room only for a selected service, so the scheme gets the whole page otherwise
		width: topologyPage.selectedService !== "" ? 250 : 0;
		visible: topologyPage.selectedService !== "";

		Component.onCompleted: {
			registerViewDelegate("ExternPaths", metaInfoViewExternPathsDelegateComp)
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

		MessageDialog {
			width: Style.sizeHintM
			title: qsTr("Deleting a service")
			message: qsTr("Delete the selected service?")
			onFinished: {
				if (buttonId == Enums.yes && topologyPage.selectedService !== ""){
					if (!topologyPage.canRemoveSelectedService()){
						topologyPage.showCannotRemoveWhileDisconnected()
						return
					}
					collectionDataProvider.removeElements([topologyPage.selectedService])
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
		id: newGroupDialogComp

		InputDialog {
			title: qsTr("New group")
			message: qsTr("Group name")
			placeHolderText: qsTr("Enter the group name")
			onFinished: {
				let name = inputValue ? inputValue.trim() : ""
				if (buttonId == Enums.ok && name !== ""){
					let position = topologyPage.findFreePosition(topologyPage.scheme ? topologyPage.scheme.objectsModel : null)
					let parentId = topologyPage.isGroupNode(topologyPage.groupId) ? topologyPage.groupId : ""
					topologyPage.topologyViewModel.createGroup(name, parentId, position.x, position.y)
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

			property string nodeId: ""
			property var targetGroups: []
			property string chosenGroupId: ""

			Component.onCompleted: {
				addButton(Enums.apply, qsTr("Add"), false)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			onFinished: {
				if (buttonId === Enums.apply && nodeId !== "" && chosenGroupId !== ""){
					topologyPage.topologyViewModel.moveNodes([nodeId], chosenGroupId)
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
