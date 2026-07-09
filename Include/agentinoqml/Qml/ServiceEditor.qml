import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentinoServicesSdl 1.0

DocumentViewBase {
	id: serviceEditorContainer
	
	anchors.fill: parent
	contentColor: Style.backgroundColor2
	
	property ServiceData serviceData: model
	
	property bool pluginLoaded: false
	property string pluginServicePath: ""
	
	function requestLoadPlugin() {
		pluginLoaded = false
		loadPlugin()
	}
	
	function setPluginLoaded(path) {
		pluginServicePath = path
		pluginLoaded = true
	}
	
	function handlePluginPathChange(newPath) {
		if (newPath !== pluginServicePath) {
			if (pluginLoaded) {
				pluginLoaded = false
				ModalDialogManager.showWarningDialog(qsTr("You have changed the Path for which the plugin was loaded, reload the plugin"))
			}
		} else {
			pluginLoaded = true
		}
	}
	
	Component.onCompleted: {
		let ok = PermissionsController.checkPermission("ChangeService");
		serviceEditorContainer.readOnly = !ok;
		multiPageView.addServicePages()
	}
	
	signal loadPlugin()
	signal pluginLoadingFailed()
	signal loadSettings()
	signal saveSettings(string content)
	
	property bool isNewService: !serviceData || serviceData.m_id === ""
	property bool serviceRunning: false
	property bool operationInProgress: false
	
	property bool settingsFileExists: false
	property string settingsPath: ""
	property string settingsContent: ""
	
	function setSettings(content, exists, path){
		settingsFileExists = exists;
		settingsPath = path;
		settingsContent = content;
		let index = multiPageView.getIndexById("Settings");
		if (index < 0){
			return;
		}
		multiPageView.ensurePageLoaded(index);
		let item = multiPageView.getPageByIndex(index);
		if (item){
			item.setContent(content);
		}
	}
	
	property string serviceTypeId: serviceData ? serviceData.m_serviceTypeId : ""
	onServiceTypeIdChanged: {
		if (serviceTypeId !== "" && multiPageView.getIndexById("Administration") < 0) {
			multiPageView.addPage("Administration", qsTr("Administration"), administrationViewComp, "Icons/AdminPanel")
		}
	}
	
	onServiceDataChanged: {
		if (serviceData){
			serviceRunning = serviceData.m_status === "running"
			operationInProgress = false
			if (pluginServicePath === "" && serviceData.hasPath()){
				pluginServicePath = serviceData.m_path
			}
			if (serviceData.hasInputConnections() && serviceData.m_inputConnections.count > 0){
				setPluginLoaded(serviceData.m_path)
			}
		}
	}
	
	function setOperationInProgress(inProgress) {
		operationInProgress = inProgress
	}
	
	function updateGui(){
		let idx = multiPageView.getIndexById("General");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Connections");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Options");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Settings");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Plugin");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
	}
	
	function updateModel(){
		let idx = multiPageView.getIndexById("General");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Connections");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Options");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Settings");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Plugin");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
	}
	
	function getHeaders(){
		return {};
	}
	
	Item {
		id: statusBar
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		height: statusBarRow.height + 2 * Style.marginM
		
		Rectangle {
			anchors.fill: parent
			color: Style.backgroundColor
			border.color: Style.borderColor
			border.width: 1
		}
		
		Row {
			id: statusBarRow
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
			anchors.leftMargin: Style.marginXL
			spacing: Style.marginM
			
			Rectangle {
				width: 10
				height: 10
				radius: 5
				anchors.verticalCenter: parent.verticalCenter
				color: serviceEditorContainer.operationInProgress ? Style.grayColor : (serviceEditorContainer.serviceRunning ? Style.greenColor : Style.errorColor)
			}
			
			BaseText {
				anchors.verticalCenter: parent.verticalCenter
				text: serviceEditorContainer.operationInProgress ? qsTr("Operation in progress...") : (serviceEditorContainer.serviceRunning ? qsTr("Running") : qsTr("Stopped"))
				font.pixelSize: Style.fontSizeM
			}
		}
		
		BusyIndicator {
			anchors.verticalCenter: parent.verticalCenter
			anchors.right: parent.right
			anchors.rightMargin: Style.marginXL
			width: 20
			height: 20
			visible: serviceEditorContainer.operationInProgress
		}
	}
	
	MultiPageView {
		id: multiPageView
		anchors.top: statusBar.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		panelWidth: Style.sizeHintXXS

		function addServicePages() {
			multiPageView.clear()
			multiPageView.addPage("General", qsTr("General"), generalPageComp, "Icons/Settings")
			multiPageView.addPage("Connections", qsTr("Connections"), connectionsPageComp, "Icons/Link")
			multiPageView.addPage("Options", qsTr("Options"), optionsPageComp, "Icons/GeneralSettings")
			multiPageView.addPage("Settings", qsTr("Settings"), settingsPageComp, "Icons/Settings")
			multiPageView.addPage("Plugin", qsTr("Plugin"), pluginPageComp, "Icons/Tools")
			if (serviceTypeId !== "") {
				multiPageView.addPage("Administration", qsTr("Administration"), administrationViewComp, "Icons/AdminPanel")
			}
			multiPageView.currentIndex = 0
		}
	}
	
	
	
	Component {
		id: generalPageComp
		
		Flickable {
			id: generalFlickable
			
			anchors.fill: parent
			anchors.leftMargin: Style.marginXL
			anchors.topMargin: Style.marginXL
			anchors.rightMargin: Style.marginXL
			anchors.bottomMargin: Style.marginXL
			
			contentWidth: generalColumn.width
			contentHeight: generalColumn.height + Style.marginXL
			
			boundsBehavior: Flickable.StopAtBounds
			clip: true
			
			function updateGui(){
				nameInput.text = serviceEditorContainer.serviceData.m_name;
				descriptionInput.text = serviceEditorContainer.serviceData.m_description; 
				pathInput.text = serviceEditorContainer.serviceData.m_path;
				argumentsInput.text = serviceEditorContainer.serviceData.m_arguments;
			}
			
			function updateModel(){
				serviceEditorContainer.serviceData.m_name = nameInput.text;
				serviceEditorContainer.serviceData.m_description = descriptionInput.text;
				serviceEditorContainer.serviceData.m_path = pathInput.text;
				serviceEditorContainer.serviceData.m_arguments = argumentsInput.text;
			}
			
			Column {
				id: generalColumn
				anchors.horizontalCenter: parent.horizontalCenter
				width: Math.min(700, parent.width - 2 * Style.marginXL)
				spacing: Style.marginXL
				
				Column {
					width: parent.width
					spacing: Style.marginL
					
					GroupHeaderView {
						title: qsTr("General Information")
						width: parent.width
						groupView: generalGroup
					}
					
					GroupElementView {
						id: generalGroup
						width: parent.width
						
						TextInputElementView {
							id: nameInput
							name: qsTr("Name")
							placeHolderText: qsTr("Enter the name");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: descriptionInput;
						}
						
						TextInputElementView {
							id: descriptionInput
							name: qsTr("Description")
							placeHolderText: qsTr("Enter the description");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: pathInput;
						}
						
						TextInputElementView {
							id: pathInput
							name: qsTr("Path")
							placeHolderText: qsTr("Enter the path");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
								
								let path = serviceEditorContainer.serviceData.m_path
								serviceEditorContainer.handlePluginPathChange(path)
							}
							
							KeyNavigation.tab: argumentsInput;
						}
						
						TextInputElementView {
							id: argumentsInput
							name: qsTr("Arguments")
							placeHolderText: qsTr("Enter the arguments");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: nameInput;
						}
					}
				}
			}
		}
	}
	
	Component {
		id: connectionsPageComp
		
		Item {
			id: connectionsPageRoot
			anchors.fill: parent
			
			function updateGui(){
				if (serviceEditorContainer.pluginLoaded) {
					connectionsFlickable.updateGui()
				}
			}
			
			function updateModel(){
				if (serviceEditorContainer.pluginLoaded) {
					connectionsFlickable.updateModel()
				}
			}
			
			Column {
				anchors.centerIn: parent
				spacing: Style.marginL
				visible: !serviceEditorContainer.pluginLoaded
				width: Math.min(400, parent.width - 2 * Style.marginXL)
				
				Image {
					anchors.horizontalCenter: parent.horizontalCenter
					width: 48
					height: 48
					sourceSize.width: 48
					sourceSize.height: 48
					source: "../../../../" + Style.getIconPath("Icons/Link", Icon.State.Off, Icon.Mode.Normal)
					opacity: 0.5
				}
				
				BaseText {
					width: parent.width
					text: qsTr("Connections are not available")
					font.pixelSize: Style.fontSizeXL
					horizontalAlignment: Text.AlignHCenter
				}
				
				BaseText {
					width: parent.width
					text: qsTr("Load the plugin first on the Plugin tab to see connection settings")
					horizontalAlignment: Text.AlignHCenter
					opacity: 0.7
					wrapMode: Text.WordWrap
				}
			}
			
			Flickable {
				id: connectionsFlickable
				
				anchors.fill: parent
				anchors.leftMargin: Style.marginXL
				anchors.topMargin: Style.marginXL
				anchors.rightMargin: Style.marginXL
				anchors.bottomMargin: Style.marginXL
				
				visible: serviceEditorContainer.pluginLoaded
				
				contentWidth: connectionsBody.width
				contentHeight: connectionsBody.height + Style.marginXL
				
				boundsBehavior: Flickable.StopAtBounds
				clip: true
				
				function updateGui(){
				if (serviceEditorContainer.serviceData.m_inputConnections){
					inputConnectionListView.model = 0
					inputConnectionListView.model = serviceEditorContainer.serviceData.m_inputConnections
					
					for (let i = 0; i < inputConnectionListView.count; i++){
						let item = inputConnectionListView.itemAtIndex(i)
						if (item){
							item.updateGui()
						}
					}
				}
				
				if (serviceEditorContainer.serviceData.m_outputConnections){
					outputConnectionListView.model = 0
					outputConnectionListView.model = serviceEditorContainer.serviceData.m_outputConnections
					
					for (let i = 0; i < outputConnectionListView.count; i++){
						let item = outputConnectionListView.itemAtIndex(i)
						if (item){
							item.updateGui()
						}
					}
				}
			}
			
			function updateModel(){
				if (serviceEditorContainer.serviceData.m_inputConnections){
					for (let i = 0; i < inputConnectionListView.count; i++){
						let item = inputConnectionListView.itemAtIndex(i)
						if (item){
							item.updateModel()
						}
					}
				}
				
				if (serviceEditorContainer.serviceData.m_outputConnections){
					for (let i = 0; i < outputConnectionListView.count; i++){
						let item = outputConnectionListView.itemAtIndex(i)
						if (item){
							item.updateModel()
						}
					}
				}
			}
			
			Column {
				id: connectionsBody
				anchors.horizontalCenter: parent.horizontalCenter
				width: Math.min(700, parent.width - 2 * Style.marginXL)
				spacing: Style.marginXL
				
				BaseText {
					visible: (!inputConnectionListView || inputConnectionListView.count === 0) && (!outputConnectionListView || outputConnectionListView.count === 0)
					text: qsTr("No connections")
					font.pixelSize: Style.fontSizeXL
					horizontalAlignment: Text.AlignHCenter
					width: parent.width
				}
				
				ListView  {
					id: inputConnectionListView
					width: parent.width
					height: contentHeight
					cacheBuffer: 1000
					boundsBehavior: Flickable.StopAtBounds
					spacing: Style.marginXL
					delegate: Column {
						width: parent.width
						spacing: Style.marginL
						GroupHeaderView {
							title:  model.item.m_connectionName + qsTr(" (input)")
							width: parent.width
							visible: inputConnectionListView.count > 0
						}
						ServerConnectionParamElementView {
							id: inputConnectionEditor
							width: inputConnectionListView.width
							readOnly: serviceEditorContainer.readOnly
							
							onParamsChanged: {
								serviceEditorContainer.doUpdateModel()
							}
							
							ButtonElementView {
								id: externConnectionsView
								name: qsTr("Extern Connections")
								text: qsTr("Edit")
								onClicked: {
									ModalDialogManager.openDialog(externPortsDialogComp, {inputConnection: model.item});
								}
							}
						}
						
						function updateGui(){
							if (!model.item){
								return
							}
							
							if (model.item.m_connectionParam){
								inputConnectionEditor.host = model.item.m_connectionParam.m_host
								inputConnectionEditor.httpPort = model.item.m_connectionParam.m_httpPort
								inputConnectionEditor.wsPort = model.item.m_connectionParam.m_wsPort
								inputConnectionEditor.isSecure = model.item.m_connectionParam.m_isSecure
							}
							
							if (model.item.m_externConnectionList){
								let values = []
								for (let i = 0; i < model.item.m_externConnectionList.count; i++){
									let externConnection = model.item.m_externConnectionList.get(i).item
									if (externConnection){
										values.push((externConnection.m_connectionParam.m_isSecure == true ? "https://" : "http://") + externConnection.m_connectionParam.m_host + ":" + externConnection.m_connectionParam.m_httpPort + externConnection.m_connectionParam.m_httpPath)
									}
								}
								
								if (values.length > 0){
									externConnectionsView.description = values.join('\n')
								}
								else{
									externConnectionsView.description = ""
								}
							}
						}
						
						function updateModel(){
							if (!model.item){
								return
							}
							
							if (model.item.m_connectionParam){
								model.item.m_connectionParam.m_host = inputConnectionEditor.host
								model.item.m_connectionParam.m_httpPort = inputConnectionEditor.httpPort
								model.item.m_connectionParam.m_wsPort = inputConnectionEditor.wsPort
								model.item.m_connectionParam.m_isSecure = inputConnectionEditor.isSecure
							}
						}
						
					}
				}
				
				Component {
					id: externPortsDialogComp;
					
					ExternPortsDialog {
						property var inputConnection: null
						
						width: serviceEditorContainer.width - Style.marginS < 1200 ? serviceEditorContainer.width - Style.marginS : 1200
						
						onStarted: {
							if (inputConnection){
								if (inputConnection.m_externConnectionList){
									connectionListModel = inputConnection.m_externConnectionList.copyMe()
								}
							}
						}
						
						onFinished: {
							if (buttonId == Enums.save){
								if (inputConnection){
									inputConnection.m_externConnectionList = connectionListModel.copyMe()
									serviceEditorContainer.doUpdateModel()
									
									serviceEditorContainer.doUpdateGui()
								}
							}
						}
					}
				}
				
				GroupHeaderView {
					title: qsTr("Output Connections")
					width: parent.width
					visible: outputConnectionListView.count > 0
				}
				
				TreeItemModel {
					id: headersModel
					Component.onCompleted: {
						let index = headersModel.insertNewItem()
						headersModel.setData("id", "host", index)
						headersModel.setData("name", qsTr("Host"), index)
						
						index = headersModel.insertNewItem()
						headersModel.setData("id", "httpPort", index)
						headersModel.setData("name", qsTr("HTTP Port"), index)
						
						index = headersModel.insertNewItem()
						headersModel.setData("id", "wsPort", index)
						headersModel.setData("name", qsTr("Web Socket Port"), index)
					}
				}
				
				ListView  {
					id: outputConnectionListView
					width: parent.width
					height: contentHeight
					cacheBuffer: 1000
					boundsBehavior: Flickable.StopAtBounds
					spacing: Style.marginXL
					delegate: GroupElementView {
						id: outputDelegate
						width: outputConnectionListView.width
						
						property var connectionParam: model && model.item ? model.item.m_connectionParam : null
						property var modelData: model ? model.item : null
						
						TextInputElementView {
							name: qsTr("Service Type")
							readOnly: true
							text: model.item.m_serviceTypeId
						}
						
						TextInputElementView {
							name: qsTr("Connection Name")
							readOnly: true
							text: model.item.m_connectionName
						}
						
						TableElementView {
							id: tableElementView
							name: qsTr("Available Connections")
							description: table && table.elementsCount == 0 ? qsTr("No available connections") : qsTr("Select one of the available connections")
							onTableChanged: {
								if (!model.item){
									return
								}
								
								if (table){
									table.isMultiCheckable = false
									table.checkable = true
									table.headers = headersModel
									
									table.setColumnContentById("host", hostCellDelegateComp);
									table.setColumnContentById("httpPort", httpPortCellDelegateComp);
									table.setColumnContentById("wsPort", wsPortCellDelegateComp);
									
									table.elements = model.item.m_dependantConnectionList
								}
							}
							
							Connections {
								target: tableElementView.table
								function onCheckedItemsChanged(){
									console.log("onCheckedItemsChanged")
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							Component {
								id: hostCellDelegateComp
								
								TableCellTextDelegate{
									function getValue(){
										if (!outputDelegate.modelData){
											return ""
										}
										
										let connectionItem = outputDelegate.modelData.m_dependantConnectionList.get(rowIndex).item.m_connectionParam
										if (connectionItem){
											return connectionItem.m_host
										}
										
										return ""
									}
								}
							}
							
							Component {
								id: httpPortCellDelegateComp
								
								TableCellTextDelegate{
									function getValue(){
										if (!outputDelegate.modelData){
											return ""
										}
										
										let connectionItem = outputDelegate.modelData.m_dependantConnectionList.get(rowIndex).item.m_connectionParam
										if (connectionItem){
											return connectionItem.m_httpPort
										}
										
										return ""
									}
								}
							}
							
							Component {
								id: wsPortCellDelegateComp
								
								TableCellTextDelegate{
									function getValue(){
										if (!outputDelegate.modelData){
											return ""
										}
										
										let connectionItem = outputDelegate.modelData.m_dependantConnectionList.get(rowIndex).item.m_connectionParam
										if (connectionItem){
											return connectionItem.m_wsPort
										}
										
										return ""
									}
								}
							}
						}
						
						function updateGui(){
							if (!tableElementView.table || !model.item){
								return
							}
							
							let dependantId = model.item.m_dependantConnectionId;
							if (dependantId === ""){
								tableElementView.table.uncheckAll()
							}
							else{
								let connectionList = model.item.m_dependantConnectionList
								for (let i = 0; i < connectionList.count; i++){
									let connectionInfo = connectionList.get(i).item
									if (connectionInfo.m_id == dependantId){
										tableElementView.table.checkItem(i)
										break
									}
								}
							}
						}
						
						function updateModel(){
							if (tableElementView.table || !model.item){
								let indexes = tableElementView.table.getCheckedItems();
								if (indexes.length > 0){
									let index = indexes[0]
									let connectionList = model.item.m_dependantConnectionList
									let connectionInfo = connectionList.get(index).item
									model.item.m_dependantConnectionId = connectionInfo.m_id
									
									model.item.m_connectionParam.m_host = connectionInfo.m_connectionParam.m_host
									model.item.m_connectionParam.m_httpPort = connectionInfo.m_connectionParam.m_httpPort
									model.item.m_connectionParam.m_wsPort = connectionInfo.m_connectionParam.m_wsPort
								}
								else{
									model.item.m_dependantConnectionId = ""
									model.item.m_connectionParam.m_host = "localhost"
									model.item.m_connectionParam.m_httpPort = -1
									model.item.m_connectionParam.m_wsPort = -1
								}
							}
						}
					}
				}
			}
		}
		}
	}

	Component {
		id: optionsPageComp
			
			Flickable {
				id: optionsFlickable
				
				anchors.fill: parent
				anchors.leftMargin: Style.marginXL
				anchors.topMargin: Style.marginXL
				anchors.rightMargin: Style.marginXL
				anchors.bottomMargin: Style.marginXL
				
				contentWidth: optionsColumn.width
				contentHeight: optionsColumn.height + Style.marginXL
				
				boundsBehavior: Flickable.StopAtBounds
				clip: true
				
				function updateGui(){
					if (!serviceEditorContainer.serviceData){
						return
					}

					switchAutoStart.checked = serviceEditorContainer.serviceData.m_isAutoStart
					
					if (serviceEditorContainer.serviceData.m_tracingLevel > -1){
						switchVerboseMessage.checked = true
					}
					else{
						switchVerboseMessage.checked = false
					}
					
					tracingLevelInput.currentIndex = serviceEditorContainer.serviceData.m_tracingLevel
					
					if (serviceEditorContainer.serviceData.m_startScript !== ""){
						startScriptChecked.checked = true
						startScriptInput.text = serviceEditorContainer.serviceData.m_startScript
					}
					else{
						startScriptChecked.checked = false
						startScriptInput.text = ""
					}
					
					if (serviceEditorContainer.serviceData.m_stopScript !== ""){
						stopScriptChecked.checked = true
						stopScriptInput.text = serviceEditorContainer.serviceData.m_stopScript
					}
					else{
						stopScriptChecked.checked = false
					}
				}
				
				function updateModel(){
					if (!serviceEditorContainer.serviceData){
						return
					}

					serviceEditorContainer.serviceData.m_isAutoStart = switchAutoStart.checked
					
					if (switchVerboseMessage.checked){
						if (tracingLevelInput.currentIndex == -1){
							tracingLevelInput.currentIndex = 0
						}
						serviceEditorContainer.serviceData.m_tracingLevel = tracingLevelInput.currentIndex
					}
					else{
						serviceEditorContainer.serviceData.m_tracingLevel = -1
					}
					
					if (startScriptChecked.checked){
						serviceEditorContainer.serviceData.m_startScript = startScriptInput.text
					}
					else{
						serviceEditorContainer.serviceData.m_startScript = ""
					}
					
					if (stopScriptChecked.checked){
						serviceEditorContainer.serviceData.m_stopScript = stopScriptInput.text
					}
					else{
						serviceEditorContainer.serviceData.m_stopScript = ""
					}
				}
				
				Column {
					id: optionsColumn
					anchors.horizontalCenter: parent.horizontalCenter
					width: Math.min(700, parent.width - 2 * Style.marginXL)
					spacing: Style.marginXL
					
					Column {
						width: parent.width
						spacing: Style.marginL
						
						GroupHeaderView {
							title: qsTr("Options")
							width: parent.width
							groupView: additionalGroup
						}
						
						GroupElementView {
							id: additionalGroup
							width: parent.width
							
							SwitchElementView {
								id: switchAutoStart
								name: qsTr("Autostart (") + (switchAutoStart.checked ? qsTr("on") : qsTr("off")) + ")"
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: switchVerboseMessage
								name: qsTr("Verbose Message")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							ComboBoxElementView {
								id: tracingLevelInput
								name: qsTr("Tracing level")
								visible: switchVerboseMessage.checked
								model: TreeItemModel {
								}
								Component.onCompleted: {
									let index = model.insertNewItem()
									model.setData("id", "0", index)
									model.setData("name", "0", index)
									
									index = model.insertNewItem()
									model.setData("id", "1", index)
									model.setData("name", "1", index)
									
									index = model.insertNewItem()
									model.setData("id", "2", index)
									model.setData("name", "2", index)
									
									index = model.insertNewItem()
									model.setData("id", "3", index)
									model.setData("name", "3", index)
									
									index = model.insertNewItem()
									model.setData("id", "4", index)
									model.setData("name", "4", index)
									
									index = model.insertNewItem()
									model.setData("id", "5", index)
									model.setData("name", "5", index)
								}
								onCurrentIndexChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: startScriptChecked
								name: qsTr("Start script")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							TextInputElementView {
								id: startScriptInput
								visible: startScriptChecked.checked
								placeHolderText: qsTr("Enter the start script path")
								name: qsTr("Start Script Path")
								onEditingFinished: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: stopScriptChecked
								name: qsTr("Stop script")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							TextInputElementView {
								id: stopScriptInput
								visible: stopScriptChecked.checked
								placeHolderText: qsTr("Enter the stop script path")
								name: qsTr("Stop Script Path")
								onEditingFinished: {
									serviceEditorContainer.doUpdateModel()
								}
							}
						}
					}
				}
			}
	}
	Component {
		id: settingsPageComp
		
		Item {
			id: settingsViewItem
			anchors.fill: parent
			
			function updateGui(){
				settingsTextEdit.text = serviceEditorContainer.settingsContent
			}
			
			function updateModel(){
			}
			
			function setContent(content){
				settingsTextEdit.text = content
			}
			
			function getContent(){
				return settingsTextEdit.text
			}
			
			Component.onCompleted: {
				updateGui()
			}
			
			Column {
				id: settingsHeaderColumn
				anchors.left: parent.left
				anchors.leftMargin: Style.marginXL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginXL
				anchors.top: parent.top
				anchors.topMargin: Style.marginXL
				spacing: Style.marginM
				
				GroupHeaderView {
					title: qsTr("Settings")
					width: parent.width
				}
				
				BaseText {
					width: parent.width
					wrapMode: Text.WordWrap
					text: serviceEditorContainer.settingsPath !== ""
						? (serviceEditorContainer.settingsFileExists
							? qsTr("Settings file: ") + serviceEditorContainer.settingsPath
							: qsTr("Settings file does not exist yet: ") + serviceEditorContainer.settingsPath)
						: qsTr("No settings file is configured for this service")
				}
				
				Row {
					spacing: Style.marginM
					
					Button {
						id: loadSettingsButton
						width: 100
						height: 32
						text: qsTr("Load")
						onClicked: {
							serviceEditorContainer.loadSettings()
						}
					}
					
					Button {
						id: saveSettingsButton
						width: 100
						height: 32
						text: qsTr("Save")
						enabled: !serviceEditorContainer.readOnly
						onClicked: {
							serviceEditorContainer.saveSettings(settingsViewItem.getContent())
						}
					}
				}
			}
			
			Rectangle {
				anchors.left: parent.left
				anchors.leftMargin: Style.marginXL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginXL
				anchors.top: settingsHeaderColumn.bottom
				anchors.topMargin: Style.marginM
				anchors.bottom: parent.bottom
				anchors.bottomMargin: Style.marginXL
				
				color: Style.backgroundColor2
				border.color: Style.borderColor
				border.width: 1
				clip: true
				
				Flickable {
					id: settingsFlickable
					anchors.fill: parent
					anchors.margins: Style.marginS
					contentWidth: width
					contentHeight: settingsTextEdit.contentHeight
					boundsBehavior: Flickable.StopAtBounds
					clip: true
					
					TextEdit {
						id: settingsTextEdit
						width: settingsFlickable.width
						readOnly: serviceEditorContainer.readOnly
						selectByMouse: true
						wrapMode: TextEdit.NoWrap
						font.family: "Courier New"
						font.pixelSize: Style.fontSizeM
						color: Style.textColor
						
						onCursorRectangleChanged: {
							if (cursorRectangle.y + cursorRectangle.height > settingsFlickable.contentY + settingsFlickable.height){
								settingsFlickable.contentY = cursorRectangle.y + cursorRectangle.height - settingsFlickable.height
							}
							else if (cursorRectangle.y < settingsFlickable.contentY){
								settingsFlickable.contentY = cursorRectangle.y
							}
						}
					}
				}
			}
		}
	}
	
	Component {
		id: pluginPageComp
			
			Item {
				id: pluginPage
				anchors.fill: parent
				
				property bool pluginLoading: false
				
				function updateGui(){}
				function updateModel(){}
				
				Column {
					anchors.horizontalCenter: parent.horizontalCenter
					width: Math.min(700, parent.width - 2 * Style.marginXL)
					spacing: Style.marginL
					
					GroupHeaderView {
						title: qsTr("Plugin information")
						width: parent.width
						groupView: pluginGroup
					}
					
					GroupElementView {
						id: pluginGroup
						width: parent.width
						
						Item {
							width: parent.width
							height: pluginStatusRow.height + Style.marginL
							
							Row {
								id: pluginStatusRow
								anchors.verticalCenter: parent.verticalCenter
								anchors.left: parent.left
								anchors.leftMargin: Style.marginL
								spacing: Style.marginM
								
								Rectangle {
									width: 10
									height: 10
									radius: 5
									anchors.verticalCenter: parent.verticalCenter
									color: serviceEditorContainer.pluginLoaded ? Style.successColor : Style.borderColor
								}
								
								BaseText {
									anchors.verticalCenter: parent.verticalCenter
									text: serviceEditorContainer.pluginLoaded ? qsTr("Plugin loaded") : qsTr("Plugin not loaded")
								}
							}
						}
						
						ButtonElementView {
							id: loadPluginButton
							name: qsTr("Load Plugin")
							text: qsTr("Load")
							description: serviceEditorContainer.isNewService ? qsTr("Save the service first before loading plugin") : (!serviceEditorContainer.pluginLoaded ? qsTr("Click to load the plugin connection data") : qsTr("Plugin successfully loaded"))
							buttonEnabled: !serviceEditorContainer.isNewService && !serviceEditorContainer.pluginLoaded && !pluginPage.pluginLoading
							onClicked: {
								pluginPage.pluginLoading = true
								serviceEditorContainer.requestLoadPlugin()
							}
						}
					}
					
					BusyIndicator {
						anchors.horizontalCenter: parent.horizontalCenter
						width: 40
						height: 40
						visible: pluginPage.pluginLoading && !serviceEditorContainer.pluginLoaded
					}
				}
				
				Connections {
					target: serviceEditorContainer
					function onPluginLoadedChanged() {
						if (serviceEditorContainer.pluginLoaded) {
							pluginPage.pluginLoading = false
						}
					}
					function onPluginLoadingFailed() {
						pluginPage.pluginLoading = false
					}
				}
			}
	}

	Component {
		id: administrationViewComp;
			
			Item {
				id: administrationViewItem
				anchors.fill: parent;
				
				function getHeaders(){
					if (serviceEditorContainer.serviceTypeId === ""){
						console.error("Unable to get additional parameters. Product-ID is empty");
						return null;
					}
					
					let obj = serviceEditorContainer.getHeaders();
					obj["productId"] = serviceEditorContainer.serviceTypeId;
					obj["token"] = userTokenProvider.accessToken;
					
					return obj;
				}
				
				UserTokenProvider {
					id: userTokenProvider
					productId: serviceEditorContainer.serviceTypeId;
					isTokenGlobal: false
					
					function getHeaders(){
						return administrationViewItem.getHeaders();
					}
					
					onAccepted: {
						authorizationPage.visible = false;
						loader.sourceComponent = administrationViewDocument
					}
				}
				
				AuthorizationPage {
					id: authorizationPage
					anchors.fill: parent;
					appName: serviceEditorContainer.serviceTypeId
					canRegisterUser: false
					canRecoveryPassword: false
					onLogin: {
						userTokenProvider.authorization(login, password)
					}
				}
				
				Loader {
					id: loader
					anchors.fill: parent;
				}
				
				Rectangle {
					anchors.fill: parent
					color: Style.backgroundColor2
					visible: !serviceEditorContainer.serviceRunning
					
					Row {
						anchors.centerIn: parent
						spacing: Style.marginM
						
						Image {
							id: image
							width: 30
							height: width
							
							sourceSize.width: width
							sourceSize.height: height
							
							source: "../../../../" + Style.getIconPath("Icons/Warning", Icon.State.On, Icon.Mode.Normal)
						}
						
						BaseText {
							text: qsTr("Service not running")
							font.pixelSize: Style.fontSizeXXL
						}
					}
				}
				
				Component {
					id: administrationViewDocument
					SingleDocumentWorkspaceView {
						id: singleDocumentWorkspaceView
						anchors.fill: administrationViewItem
						documentManager: DocumentService {}
						
						visualStatusProvider: GqlBasedObjectVisualStatusProvider {
							function getHeaders(){
								return administrationViewItem.getHeaders();
							}
						}
						
						Component.onCompleted: {
							addInitialItem(administrationView, "Administration")
						}
						
						Component {
							id: administrationView;
							AdministrationView {
								anchors.fill: parent;
								productId: serviceEditorContainer.serviceTypeId;
								documentManager: singleDocumentWorkspaceView.documentManager;
								
								function getHeaders(){
									return administrationViewItem.getHeaders();
								}
							}
						}
					}
				}
				
	}
}
}
