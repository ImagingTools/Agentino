import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentinoServicesSdl 1.0

ViewBase {
	id: serviceEditorContainer;
	
	property ServiceData serviceData: model;
	
	Component.onCompleted: {
		let ok = PermissionsController.checkPermission("ChangeService");
		serviceEditorContainer.readOnly = !ok;
		tabPanel.addTab("General", qsTr("General"), mainEditorComp);
	}
	
	signal loadPlugin()
	
	property bool serviceRunning: false
	property bool pluginLoaded: false
	property string pluginServicePath: ""
	
	property string serviceTypeId: serviceData ? serviceData.m_serviceTypeId : ""
	onServiceTypeIdChanged: {
		let tabId = "Administration"
		if (tabPanel.getIndexById(tabId) < 0){
			tabPanel.addTab(tabId, qsTr("Administration"), administrationViewComp)
		}
	}
	
	onServiceDataChanged: {
		if (serviceData){
			serviceRunning = serviceData.m_status === "running"
			if (pluginServicePath === "" && serviceData.hasPath()){
				pluginServicePath = serviceData.m_path
			}
		}
	}
	
	function updateGui(){
		let item = tabPanel.getTabByIndex(0);
		item.updateGui();
	}
	
	function updateModel(){
		let item = tabPanel.getTabByIndex(0);
		item.updateModel();
	}
	
	function getHeaders(){
		return {};
	}
	
	TabView {
		id: tabPanel
		anchors.fill: parent;
		currentIndex: 0;
	}


	
	Component {
		id: mainEditorComp;
		
		Flickable {
			id: flickable;
			
			anchors.left: parent.left;
			anchors.leftMargin: Style.marginXL;
			
			anchors.top: parent.top;
			anchors.topMargin: Style.marginXL;
			
			anchors.bottom: parent.bottom;
			anchors.bottomMargin: Style.marginXL;
			
			anchors.right: scrollbar.left;
			anchors.rightMargin: Style.marginXL;
			
			contentWidth: bodyColumn.width;
			contentHeight: bodyColumn.height + 2 * Style.marginXL + 100

			boundsBehavior: Flickable.StopAtBounds;
			
			clip: true;
			
			function updateGui(){
				nameInput.text = serviceEditorContainer.serviceData.m_name;
				descriptionInput.text = serviceEditorContainer.serviceData.m_description; 
				pathInput.text = serviceEditorContainer.serviceData.m_path;
				argumentsInput.text = serviceEditorContainer.serviceData.m_arguments;
				switchAutoStart.checked = serviceEditorContainer.serviceData.m_isAutoStart;
				
				if (serviceEditorContainer.serviceData.m_tracingLevel > -1){
					switchVerboseMessage.checked = true;
				}
				else{
					switchVerboseMessage.checked = false;
				}
				
				tracingLevelInput.currentIndex = serviceEditorContainer.serviceData.m_tracingLevel;
				
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
					let count = serviceEditorContainer.serviceData.m_outputConnections.count
					
					for (let i = 0; i < outputConnectionListView.count; i++){
						let item = outputConnectionListView.itemAtIndex(i)
						if (item){
							item.updateGui()
						}
					}
				}
			}
			
			function updateModel(){
				serviceEditorContainer.serviceData.m_name = nameInput.text;
				serviceEditorContainer.serviceData.m_description = descriptionInput.text;
				serviceEditorContainer.serviceData.m_path = pathInput.text;
				serviceEditorContainer.serviceData.m_arguments = argumentsInput.text;
				serviceEditorContainer.serviceData.m_isAutoStart = switchAutoStart.checked;
				
				if (switchVerboseMessage.checked){
					if (tracingLevelInput.currentIndex == -1){
						tracingLevelInput.currentIndex = 0;
					}
					
					serviceEditorContainer.serviceData.m_tracingLevel = tracingLevelInput.currentIndex;
				}
				else{
					serviceEditorContainer.serviceData.m_tracingLevel = -1;
				}
				
				if(startScriptChecked.checked){
					serviceEditorContainer.serviceData.m_startScript = startScriptInput.text;
				}
				else{
					serviceEditorContainer.serviceData.m_startScript = "";
				}
				
				if(stopScriptChecked.checked){
					serviceEditorContainer.serviceData.m_stopScript = stopScriptInput.text;
				}
				else{
					serviceEditorContainer.serviceData.m_stopScript = "";
				}
				
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
			
			CustomScrollbar {
				id: scrollbar;
				
				anchors.left: flickable.right;
				anchors.leftMargin: Style.marginXS;
				anchors.top: parent.top;
				anchors.bottom: flickable.parent.bottom;
				
				secondSize: Style.marginM;
				targetItem: flickable;
				
				visible: parent.visible;
			}
			
			Column {
				id: bodyColumn;
				width: 700
				spacing: Style.marginXL;
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
							if (path === serviceEditorContainer.pluginServicePath){
								serviceEditorContainer.pluginLoaded = true
							}
							else{
								if (serviceEditorContainer.pluginLoaded){
									serviceEditorContainer.pluginLoaded = false
									ModalDialogManager.showWarningDialog(qsTr("You have changed the Path for which the plugin was loaded, reload the plugin"))
								}
							}
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
				
				GroupHeaderView {
					title: qsTr("Additional Information")
					width: parent.width
					groupView: additionalGroup
				}
				
				GroupElementView {
					id: additionalGroup
					width: parent.width
					
					SwitchElementView {
						id: switchAutoStart
						name: qsTr("Autostart (") + (switchAutoStart.checked ? qsTr("on") : qsTr("off")) + ")";
						onCheckedChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					SwitchElementView {
						id: switchVerboseMessage
						name: qsTr("Verbose Message")
						onCheckedChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					ComboBoxElementView {
						id: tracingLevelInput
						name: qsTr("Tracing level");
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
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					SwitchElementView {
						id: startScriptChecked
						name: qsTr("Start script")
						onCheckedChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					TextInputElementView {
						id: startScriptInput
						visible: startScriptChecked.checked
						placeHolderText: qsTr("Enter the start script path");
						name: qsTr("Start Script Path")
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					SwitchElementView {
						id: stopScriptChecked
						name: qsTr("Stop script")
						onCheckedChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					TextInputElementView {
						id: stopScriptInput
						visible: stopScriptChecked.checked
						placeHolderText: qsTr("Enter the stop script path");
						name: qsTr("Stop Script Path")
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
					}
				}
				
				GroupHeaderView {
					title: qsTr("Plugin information")
					width: parent.width
					groupView: pluginGroup
				}
				
				GroupElementView {
					id: pluginGroup
					width: parent.width
					ButtonElementView {
						id: loadPluginButton
						name: qsTr("Load Plugin")
						text: qsTr("Load")
						description: buttonEnabled ? qsTr("Click if you want to load the plugin data") : qsTr("Plugin succesfully loaded")
						buttonEnabled: !serviceEditorContainer.pluginLoaded
						onClicked: {
							serviceEditorContainer.loadPlugin()
						}
					}
				}

				GroupHeaderView {
					title: qsTr("Input Connections")
					width: parent.width
					visible: inputConnectionListView.count > 0
				}
				
				ListView  {
					id: inputConnectionListView
					width: parent.width
					height: contentHeight
					cacheBuffer: 1000
					boundsBehavior: Flickable.StopAtBounds
					spacing: Style.marginXL
					delegate: ServerConnectionParamElementView {
						id: inputConnectionEditor
						width: inputConnectionListView.width
						readOnly: serviceEditorContainer.readOnly
						
						Component.onCompleted: {
							updateGui()
						}
						
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
										values.push(externConnection.m_connectionParam.m_host + ":" + externConnection.m_connectionParam.m_httpPort)
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
			}//Column bodyColumn
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
					documentManager: DocumentManager {}
					
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
}//serviceEditorContainer

