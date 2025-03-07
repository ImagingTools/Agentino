import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentinoServicesSdl 1.0

ViewBase {
	id: serviceEditorContainer;
	
	property int radius: 3;
	property int flickableWidth: 800;
	property string productId;
	property string productName;
	property string agentId;
	property var documentManager: null;
	
	property ServiceData serviceData: model;
	
	Component.onCompleted: {
		let ok = PermissionsController.checkPermission("ChangeService");
		
		serviceEditorContainer.readOnly = !ok;
		
		tabPanel.addTab("General", qsTr("General"), mainEditorComp);
	}
	
	onProductIdChanged: {
		if (productId !== ""){
			tabPanel.addTab("Administration", qsTr("Administration"), administrationViewComp);
		}
	}
	
	onServiceDataChanged: {
		if (serviceData){
			// TODO Name???
			serviceEditorContainer.productId = serviceData.m_name;
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
			
			anchors.fill: parent
			
			contentWidth: bodyContainer.width;
			contentHeight: bodyContainer.height + 200;
			
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
					startScriptChecked.checkState = Qt.Checked
					startScriptInput.text = serviceEditorContainer.serviceData.m_startScript
				}
				else{
					startScriptChecked.checkState = Qt.Unchecked
					startScriptInput.text = ""
				}
				
				if (serviceEditorContainer.serviceData.m_stopScript !== ""){
					stopScriptChecked.checkState = Qt.Checked
					stopScriptInput.text = serviceEditorContainer.serviceData.m_stopScript
				}
				else{
					stopScriptChecked.checkState = Qt.Unchecked
				}
				
				inputConnTable.elements = serviceEditorContainer.serviceData.m_inputConnections;
				ouputConnTable.elements = serviceEditorContainer.serviceData.m_outputConnections;
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
				
				if(startScriptChecked.checkState === Qt.Checked){
					serviceEditorContainer.serviceData.m_startScript = startScriptInput.text;
				}
				else{
					serviceEditorContainer.serviceData.m_startScript = "";
				}
				
				if(stopScriptChecked.checkState === Qt.Checked){
					serviceEditorContainer.serviceData.m_stopScript = stopScriptInput.text;
				}
				else{
					serviceEditorContainer.serviceData.m_stopScript = "";
				}
			}
			
			CustomScrollbar {
				id: scrollbar;
				
				anchors.left: flickable.right;
				anchors.leftMargin: Style.sizeSmallMargin;
				anchors.top: parent.top;
				anchors.bottom: parent.bottom;
				
				secondSize: Style.sizeMainMargin;
				targetItem: flickable;
				
				visible: parent.visible;
			}
			
			Item {
				id: bodyContainer
				width: flickable.width
				height: bodyColumn.height + Style.sizeLargeMargin * 2
				
				Column {
					id: bodyColumn;
					
					anchors.top: bodyContainer.top
					anchors.left: bodyContainer.left;
					anchors.right: bodyContainer.right
					anchors.margins: Style.sizeLargeMargin
					
					spacing: Style.sizeMainMargin;
					
					Text {
						id: titleName;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Name");
					}
					
					CustomTextField {
						id: nameInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						autoEditingFinished: false
						
						placeHolderText: qsTr("Enter the name");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: descriptionInput;
					}
					
					Text {
						id: descriptionName;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Description");
					}
					
					CustomTextField {
						id: descriptionInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						
						placeHolderText: qsTr("Enter the description");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: pathInput;
					}
					
					Text {
						id: titlePath;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Path");
					}
					
					CustomTextField {
						id: pathInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						
						placeHolderText: qsTr("Enter the path");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: argumentsInput;
					}
					
					Text {
						id: titleArguments;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Arguments");
					}
					
					CustomTextField {
						id: argumentsInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						
						placeHolderText: qsTr("Enter the arguments");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: nameInput;
					}
					
					Text {
						id: titleAutoStart;
						
						anchors.left: parent.left;
						
						width: parent.width;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Autostart (") + (switchAutoStart.checked ? qsTr("on") : qsTr("off")) + ")";
					}
					
					SwitchCustom {
						id: switchAutoStart
						
						backgroundColor: "#D4D4D4"
						onCheckedChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					Text {
						id: titleVerboseMessage;
						
						anchors.left: parent.left;
						
						width: parent.width;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Verbose message (") + (switchVerboseMessage.checked ? qsTr("on") : qsTr("off")) + ")";
					}
					
					Row {
						height:  Style.itemSizeMedium;
						spacing: Style.sizeMainMargin;
						
						SwitchCustom {
							id: switchVerboseMessage
							anchors.verticalCenter: parent.verticalCenter
							
							backgroundColor: "#D4D4D4"
							onCheckedChanged: {
								serviceEditorContainer.doUpdateModel();
							}
						}
						
						Item {
							width: Style.sizeMainMargin;
							height: Style.itemSizeMedium
						}
						
						Text {
							id: titleTracingLevel;
							anchors.verticalCenter: parent.verticalCenter
							
							visible: switchVerboseMessage.checked
							color: Style.textColor;
							font.family: Style.fontFamily;
							font.pixelSize: Style.fontSizeNormal;
							
							text: qsTr("Tracing level");
						}
						
						ComboBox {
							id: tracingLevelInput
							anchors.verticalCenter: parent.verticalCenter
							height: Style.itemSizeMedium * 0.75;
							width: Style.itemSizeLarge;
							visible: switchVerboseMessage.checked
							
							model: TreeItemModel {
							}
							Component.onCompleted: {
								let index = model.insertNewItem()
								model.setData("Name", "0", index)
								
								index = model.insertNewItem()
								model.setData("Name", "1", index)
								
								index = model.insertNewItem()
								model.setData("Name", "2", index)
								
								index = model.insertNewItem()
								model.setData("Name", "3", index)
								
								index = model.insertNewItem()
								model.setData("Name", "4", index)
								
								index = model.insertNewItem()
								model.setData("Name", "5", index)
							}
							onCurrentIndexChanged: {
								serviceEditorContainer.doUpdateModel();
							}
						}
					}
					
					CheckBox {
						id: startScriptChecked
						text: qsTr("Start script")
						onCheckStateChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					CustomTextField {
						id: startScriptInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						visible: startScriptChecked.checkState === Qt.Checked
						
						placeHolderText: qsTr("Enter the start script path");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: nameInput;
					}
					
					CheckBox {
						id: stopScriptChecked
						text: qsTr("Stop script")
						onCheckStateChanged: {
							serviceEditorContainer.doUpdateModel();
						}
					}
					
					CustomTextField {
						id: stopScriptInput;
						
						width: parent.width;
						height: Style.itemSizeMedium;
						visible: stopScriptChecked.checkState === Qt.Checked
						
						placeHolderText: qsTr("Enter the stop script path");
						
						onEditingFinished: {
							serviceEditorContainer.doUpdateModel();
						}
						
						KeyNavigation.tab: nameInput;
					}
					
					Text {
						id: inputConnectionsTitle;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Incoming Connections");
						
						visible: false;
					}
					
					Table {
						id: inputConnTable;
						
						width: parent.width;
						height: contentHeight + headerHeight + 15;
						
						canMoveColumns: true;
						canFitHeight: true;
						wrapMode_deleg: Text.WordWrap;
						
						radius: 0;
						selectable: false
						
						visible: false;
						
						onElementsChanged: {
							let count = elements.getItemsCount();
							
							inputConnectionsTitle.visible = count > 0;
							inputConnTable.visible = count > 0;
						}
						
						onHeadersChanged: {
							inputConnTable.setColumnContentById("m_description", textInputComp)
							inputConnTable.setColumnContentById("m_port", portInputComp)
							inputConnTable.setColumnContentById("m_externPorts", externCompEditComp)
							
							inputConnTable.tableDecorator = inputTableDecoratorModel;
						}
						
						onSelectionChanged: {
							ouputConnTable.resetSelection();
						}
						
						TreeItemModel {
							id: headersModel;
							
							Component.onCompleted: {
								let index = headersModel.insertNewItem();
								
								headersModel.setData("Id", "m_connectionName", index)
								headersModel.setData("Name", qsTr("Usage"), index)
								
								index = headersModel.insertNewItem();
								
								headersModel.setData("Id", "m_description", index)
								headersModel.setData("Name", qsTr("Description"), index)
								
								index = headersModel.insertNewItem();
								
								headersModel.setData("Id", "m_port", index)
								headersModel.setData("Name", qsTr("Port"), index)
								
								index = headersModel.insertNewItem();
								
								headersModel.setData("Id", "m_externPorts", index)
								headersModel.setData("Name", qsTr("Extern Addresses"), index)
								
								inputConnTable.headers = headersModel;
							}
						}
						
						TreeItemModel {
							id: inputTableDecoratorModel;
							
							Component.onCompleted: {
								var cellWidthModel = inputTableDecoratorModel.addTreeModel("CellWidth");
								
								let index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 150, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 300, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 100, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", -1, index);
							}
						}
						
						Component {
							id: textInputComp;
							
							TextInputCellContentComp {
								id: textInputComp;
							}
						}
						
						Component {
							id: portInputComp;
							
							TextInputCellContentComp {
								id: textInputContent;
								
								function getValue(){
									return rowDelegate.dataModel.item.m_url.m_port;
								}
								
								function setValue(value){
									let urlParam = rowDelegate.dataModel.item.m_url;
									urlParam.m_port = value;
									rowDelegate.dataModel.item.m_url = urlParam;
								}
							}
						}
						
						Component {
							id: externCompEditComp;
							TableCellDelegateBase {
								id: content;
								
								onReused: {
									if (rowIndex >= 0){
										let valueModel = getValue();
										if (valueModel){
											let values = []
											for (let i = 0; i < valueModel.count; i++){
												let item = valueModel.get(i).item;
												values.push(item.m_host + ":" + item.m_port)
											}
											textLabel.text = values.join('\n')
										}
									}
								}
								
								Text {
									id: textLabel;
									
									anchors.verticalCenter: parent.verticalCenter;
									anchors.left: parent.left;
									anchors.right: button.left;
									
									elide: Text.ElideRight;
									wrapMode: Text.WordWrap;
									
									clip: true;
									
									color: Style.textColor;
									font.family: Style.fontFamily;
									font.pixelSize: Style.fontSizeNormal;
									lineHeight: 1.5;
									
									onTextChanged: {
										if (content.tableCellDelegate){
											let columnIndex = content.tableCellDelegate.columnIndex;
											content.tableCellDelegate.pTableDelegateContainer.setHeightModelElememt(columnIndex, textLabel.height);
											content.tableCellDelegate.pTableDelegateContainer.setCellHeight();
										}
									}
								}
								
								ToolButton {
									id: button;
									
									anchors.verticalCenter: parent.verticalCenter;
									anchors.right: parent.right;
									anchors.rightMargin: Style.sizeMainMargin
									
									width: 18;
									height: width;
									
									iconSource: "../../../../" + Style.getIconPath("Icons/Edit", Icon.State.Off, Icon.Mode.Normal);
									
									visible: !serviceEditorContainer.readOnly;
									
									onClicked: {
										ModalDialogManager.openDialog(externPortsDialogComp, {});
									}
								}
								
								Component {
									id: externPortsDialogComp;
									
									ExternPortsDialog {
										onStarted: {
											let item = inputConnTable.elements.get(content.rowIndex).item;
											setPortsModel(item.m_externPorts);
										}
										
										onFinished: {
											if (buttonId == Enums.save){
												if (content.rowIndex >= 0){
													let ports = []
													for (let i = 0; i < portsModel.count; i++){
														let item = portsModel.get(i).item;
														ports.push(item.m_host + ":" + item.m_port)
													}
													
													
													let externPortsModel = inputConnTable.elements.get(content.rowIndex).item.m_externPorts;
													externPortsModel.clear();
													for (let i = 0; i < portsModel.count; i++){
														externPortsModel.addElement(portsModel.get(i).item)
													}
													
													textLabel.text = ports.join('\n');
												}
											}
										}
									}
								}
							}
						}
					}
					
					Text {
						id: dependantServicesTitle;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeNormal;
						
						text: qsTr("Dependant Services");
						
						visible: false;
					}
					
					Table {
						id: ouputConnTable;
						
						width: parent.width;
						height: visible ? contentHeight + headerHeight + 15 : 0;
						
						canMoveColumns: true;
						radius: 0;
						selectable: false;
						
						visible: false;
						
						onHeadersChanged: {
							ouputConnTable.setColumnContentComponent(2, textInputComp2)
							ouputConnTable.setColumnContentComponent(3, comboBoxComp2)
							
							ouputConnTable.tableDecorator = outputTableDecoratorModel;
						}
						
						onElementsChanged: {
							let count = elements.getItemsCount();
							
							dependantServicesTitle.visible = count > 0;
							ouputConnTable.visible = count > 0;
						}
						
						Component {
							id: textInputComp2;
							TextInputCellContentComp {
							}
						}
						
						Component {
							id: comboBoxComp2;
							
							TableCellDelegateBase {
								id: bodyItem;
								
								onReused: {
									if (rowIndex >= 0){
										let value = getValue();
										let item = ouputConnTable.elements.get(bodyItem.rowIndex).item;
										
										let dependantConnectionId = item.m_dependantConnectionId;
										let elementsModel = item.m_elements;
										
										textLabel.text = value;
										cb.model = elementsModel;
										
										console.log("cb.model", cb.model);
										
										if (cb.model){
											for (let i = 0; i < cb.model.count; i++){
												if (cb.model.get(i).item.m_id == dependantConnectionId){
													cb.currentIndex = i;
													break;
												}
											}
										}
									}
								}
								
								Text {
									id: textLabel;
									
									anchors.verticalCenter: parent.verticalCenter;
									
									width: parent.width;
									
									elide: Text.ElideRight;
									wrapMode: Text.NoWrap;
									
									color: Style.textColor;
									font.family: Style.fontFamily;
									font.pixelSize: Style.fontSizeNormal;
								}
								
								ComboBox {
									id: cb;
									
									width: parent.width;
									height: 25;
									nameId: "m_name";
									
									visible: false;
									
									onCurrentIndexChanged: {
										cb.visible = false;
										if (cb.model){
											let item = cb.model.get(cb.currentIndex).item;
											let id = item.m_id
											let name = item.m_name
											
											textLabel.text = name;
											
											let outputItem = ouputConnTable.elements.get(bodyItem.rowIndex).item;
											outputItem.m_dependantConnectionId = id;
										}
									}
									
									onFinished: {
										cb.visible = false;
									}
									
									onFocusChanged: {
										if (!focus){
											cb.visible = false;
										}
									}
								}
								
								MouseArea {
									id: ma;
									
									anchors.fill: parent;
									
									visible: !serviceEditorContainer.readOnly;
									
									onDoubleClicked: {
										console.log("onDoubleClicked", cb.model);
										if (cb.model && cb.model.count > 0){
											cb.visible = true;
											cb.z = ma.z + 1;
											cb.forceActiveFocus();
											cb.openPopupMenu();
										}
									}
								}
							}
						}
						
						TreeItemModel {
							id: headersModel2;
							
							Component.onCompleted: {
								let index = headersModel2.insertNewItem();
								
								headersModel2.setData("Id", "m_connectionName", index)
								headersModel2.setData("Name", qsTr("Usage"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("Id", "m_serviceTypeName", index)
								headersModel2.setData("Name", qsTr("Service"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("Id", "m_description", index)
								headersModel2.setData("Name", qsTr("Description"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("Id", "m_displayUrl", index)
								headersModel2.setData("Name", qsTr("Url"), index)
								
								ouputConnTable.headers = headersModel2;
							}
						}
						
						TreeItemModel {
							id: outputTableDecoratorModel;
							
							Component.onCompleted: {
								var cellWidthModel = outputTableDecoratorModel.addTreeModel("CellWidth");
								
								let index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 150, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 80, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", 350, index);
								
								index = cellWidthModel.insertNewItem();
								cellWidthModel.setData("Width", -1, index);
							}
						}
					}
				}//Column bodyColumn
			}
		}
	}
	
	Component {
		id: administrationViewComp;
		
		Item {
			id: administrationViewItem
			anchors.fill: parent;
			
			function getHeaders(){
				if (serviceEditorContainer.productId === ""){
					console.error("Unable to get additional parameters. Product-ID is empty");
					return null;
				}
				
				let obj = serviceEditorContainer.getHeaders();
				obj["ProductId"] = serviceEditorContainer.productId;
				obj["token"] = userTokenProvider.token;
				
				return obj;
			}
			
			UserTokenProvider {
				id: userTokenProvider
				productId: serviceEditorContainer.productId;
				isTokenGlobal: false
				
				function getHeaders(){
					return administrationViewItem.getHeaders();
				}
				
				onAccepted: {
					authorizationPage.visible = false;
					loader.sourceComponent = administrationViewDocument
				}
				
				onFailed: {
					root.loginFailed();
				}
			}
			
			AuthorizationPage {
				id: authorizationPage
				anchors.fill: parent;
				appName: serviceEditorContainer.productId
				function onLogin(login, password){
					userTokenProvider.authorization(login, password)
				}
			}
			
			Loader {
				id: loader
				anchors.fill: parent;
			}
			
			Component {
				id: administrationViewDocument
				SingleDocumentWorkspaceView {
					id: singleDocumentWorkspaceView
					anchors.fill: administrationViewItem
					documentManager: DocumentManager {}
					
					Component.onCompleted: {
						addInitialItem(administrationView, "Administration")
					}
					
					Component {
						id: administrationView;
						AdministrationView {
							anchors.fill: parent;
							productId: serviceEditorContainer.productId;
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

