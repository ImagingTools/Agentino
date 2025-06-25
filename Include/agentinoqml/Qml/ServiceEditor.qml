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
	
	property bool serviceRunning: false

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
				
				if (!inputConnTable.elements){
					if (serviceEditorContainer.serviceData.m_inputConnections){
						inputConnTable.elements = serviceEditorContainer.serviceData.m_inputConnections.copyMe()
					}
				}
				else{
					for (let inputItemIndex = 0; inputItemIndex < inputConnTable.elements.count; inputItemIndex++){
						let inputItem = inputConnTable.elements.get(inputItemIndex).item
						let originalItem = serviceEditorContainer.serviceData.m_inputConnections.get(inputItemIndex).item
						
						inputItem.m_description = originalItem.m_description
						
						if (originalItem.m_connectionParam){
							inputItem.m_connectionParam = originalItem.m_connectionParam.copyMe()
						}

						if (originalItem.m_externPorts){
							inputItem.m_externPorts = originalItem.m_externPorts.copyMe()
						}
					}
				}
				
				if (!ouputConnTable.elements){
					if (serviceEditorContainer.serviceData.m_outputConnections){
						ouputConnTable.elements = serviceEditorContainer.serviceData.m_outputConnections.copyMe()
					}
				}
				else{
					for (let outputItemIndex = 0; outputItemIndex < ouputConnTable.elements.count; outputItemIndex++){
						let outputItem = ouputConnTable.elements.get(outputItemIndex).item
						let originalItem = serviceEditorContainer.serviceData.m_outputConnections.get(outputItemIndex).item
						if (originalItem.m_connectionParam){
							outputItem.m_connectionParam = originalItem.m_connectionParam.copyMe()
						}

						outputItem.m_description = originalItem.m_description
						outputItem.m_dependantConnectionId = originalItem.m_dependantConnectionId
					}
				}
			}
			
			function updateModel(){
				console.log("updateModel begin")
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

				if (inputConnTable.elements){
					for (let inputItemIndex = 0; inputItemIndex < inputConnTable.elements.count; inputItemIndex++){
						let inputConnections = serviceEditorContainer.serviceData.m_inputConnections
						let inputItem = inputConnTable.elements.get(inputItemIndex).item
						
						if (inputConnections.get(inputItemIndex).item.m_description !== inputItem.m_description){
							inputConnections.setProperty(inputItemIndex, "m_description", inputItem.m_description);
						}
						
						if (inputItem.m_connectionParam){
							if (!inputConnections.get(inputItemIndex).item.m_connectionParam.isEqualWithModel(inputItem.m_connectionParam)){
								inputConnections.setProperty(inputItemIndex, "m_connectionParam", inputItem.m_connectionParam.copyMe());
							}
						}

						// if (!inputConnections.get(inputItemIndex).item.m_externPorts.isEqualWithModel(inputItem.m_externPorts)){
						// 	inputConnections.setProperty(inputItemIndex, "m_externPorts", inputItem.m_externPorts.copyMe());
						// }
					}
				}
				
				if (ouputConnTable.elements){
					for (let outputItemIndex = 0; outputItemIndex < ouputConnTable.elements.count; outputItemIndex++){
						let outputItem = ouputConnTable.elements.get(outputItemIndex).item
						let outputConnections = serviceEditorContainer.serviceData.m_outputConnections
						
						if (outputConnections.get(outputItemIndex).item.m_description !== outputItem.m_description){
							outputConnections.setProperty(outputItemIndex, "m_description", outputItem.m_description);
						}
						
						if (outputConnections.get(outputItemIndex).item.m_dependantConnectionId !== outputItem.m_dependantConnectionId){
							outputConnections.setProperty(outputItemIndex, "m_dependantConnectionId", outputItem.m_dependantConnectionId);
						}
						
						if (outputItem.m_connectionParam){
							if (!outputConnections.get(outputItemIndex).item.m_connectionParam.isEqualWithModel(outputItem.m_connectionParam)){
								outputConnections.setProperty(outputItemIndex, "m_connectionParam", outputItem.m_connectionParam.copyMe());
							}
						}
					}
				}
				console.log("updateModel end")
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
			
			Item {
				id: bodyContainer
				width: flickable.width
				height: bodyColumn.height + Style.marginXL * 2
				
				Column {
					id: bodyColumn;
					
					anchors.top: bodyContainer.top
					anchors.left: bodyContainer.left;
					anchors.right: bodyContainer.right
					anchors.margins: Style.marginXL
					
					spacing: Style.marginM;
					
					Text {
						id: titleName;
						
						anchors.left: parent.left;
						
						color: Style.textColor;
						font.family: Style.fontFamily;
						font.pixelSize: Style.fontSizeM;
						
						text: qsTr("Name");
					}
					
					CustomTextField {
						id: nameInput;
						
						width: parent.width;
						height: Style.itemSizeM;
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
						font.pixelSize: Style.fontSizeM;
						
						text: qsTr("Description");
					}
					
					CustomTextField {
						id: descriptionInput;
						
						width: parent.width;
						height: Style.itemSizeM;
						
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
						font.pixelSize: Style.fontSizeM;
						
						text: qsTr("Path");
					}
					
					CustomTextField {
						id: pathInput;
						
						width: flickable.parent.width;
						height: Style.itemSizeM;
						
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
						font.pixelSize: Style.fontSizeM;
						
						text: qsTr("Arguments");
					}
					
					CustomTextField {
						id: argumentsInput;
						
						width: parent.width;
						height: Style.itemSizeM;
						
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
						font.pixelSize: Style.fontSizeM;
						
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
						font.pixelSize: Style.fontSizeM;
						
						text: qsTr("Verbose message (") + (switchVerboseMessage.checked ? qsTr("on") : qsTr("off")) + ")";
					}
					
					Row {
						height:  Style.itemSizeM;
						spacing: Style.marginM;
						
						SwitchCustom {
							id: switchVerboseMessage
							anchors.verticalCenter: parent.verticalCenter
							onCheckedChanged: {
								serviceEditorContainer.doUpdateModel();
							}
						}
						
						Item {
							width: Style.marginM;
							height: Style.itemSizeM
						}
						
						Text {
							id: titleTracingLevel;
							anchors.verticalCenter: parent.verticalCenter
							
							visible: switchVerboseMessage.checked
							color: Style.textColor;
							font.family: Style.fontFamily;
							font.pixelSize: Style.fontSizeM;
							
							text: qsTr("Tracing level");
						}
						
						ComboBox {
							id: tracingLevelInput
							anchors.verticalCenter: parent.verticalCenter
							height: Style.itemSizeM * 0.75;
							width: Style.itemSizeL;
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
						height: Style.itemSizeM;
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
						height: Style.itemSizeM;
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
						font.pixelSize: Style.fontSizeM;
						
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
							inputConnTable.setColumnContentById("connectionName", textInputComp)
							inputConnTable.setColumnContentById("wsPort", wsPortInputComp)
							inputConnTable.setColumnContentById("httpPort", httpPortInputComp)
							inputConnTable.setColumnContentById("isSecure", secureColumnComp)
							// inputConnTable.setColumnContentById("externPorts", externCompEditComp)
							
							// inputConnTable.tableDecorator = inputTableDecoratorModel;
						}
						
						onSelectionChanged: {
							ouputConnTable.resetSelection();
						}
						
						TreeItemModel {
							id: headersModel;
							
							Component.onCompleted: {
								let index = headersModel.insertNewItem();
								
								headersModel.setData("id", "connectionName", index)
								headersModel.setData("name", qsTr("Connection Name"), index)

								index = headersModel.insertNewItem();
								
								headersModel.setData("id", "wsPort", index)
								headersModel.setData("name", qsTr("Web Socket Port"), index)
								
								index = headersModel.insertNewItem();
								
								headersModel.setData("id", "httpPort", index)
								headersModel.setData("name", qsTr("Http Port"), index)
								
								index = headersModel.insertNewItem();
								
								headersModel.setData("id", "isSecure", index)
								headersModel.setData("name", qsTr("Secure Connection"), index)
								
								// index = headersModel.insertNewItem();
								
								// headersModel.setData("id", "externPorts", index)
								// headersModel.setData("name", qsTr("Extern Addresses"), index)
								
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
								onEditingFinished: {
									let actual = serviceEditorContainer.serviceData.m_inputConnections.get(rowIndex).item.m_description
									if (actual != text){
										serviceEditorContainer.doUpdateModel()
									}
								}
							}
						}
						
						Component {
							id: secureColumnComp;
							
							TableCellDelegateBase {
								id: secureColumnCellDelegate
								SwitchCustom {
									id: switchCustom
									onCheckedChanged: {
										if (secureColumnCellDelegate.rowIndex >= 0){
											serviceEditorContainer.serviceData.m_inputConnections.get(secureColumnCellDelegate.rowIndex).item.m_connectionParam.m_isSecure = checked
										}
									}
								}
								
								onReused: {
									if (rowIndex >= 0){
										let isSecure = serviceEditorContainer.serviceData.m_inputConnections.get(rowIndex).item.m_connectionParam.m_isSecure
										switchCustom.checked = isSecure
									}
								}
							}
						}
						
						Component {
							id: httpPortInputComp;
							
							TextInputCellContentComp {
								id: textInputContent;
								
								property int port: rowDelegate && rowDelegate.dataModel ? rowDelegate.dataModel.item.m_connectionParam.m_httpPort : -1
								onPortChanged: {
									reused()
								}
								
								onEditingFinished: {
									console.log("port onEditingFinished", port)
									let actualPort = serviceEditorContainer.serviceData.m_inputConnections.get(rowIndex).item.m_connectionParam.m_httpPort
									if (actualPort !== port){
										serviceEditorContainer.doUpdateModel()
									}
								}
								
								function getValue(){
									return rowDelegate.dataModel.item.m_connectionParam.m_httpPort
								}
								
								function setValue(value){
									console.log("port setValue", value)
									let urlParam = rowDelegate.dataModel.item.m_connectionParam;
									urlParam.m_httpPort = value;
									rowDelegate.dataModel.item.m_connectionParam = urlParam;
								}
							}
						}
						
						Component {
							id: wsPortInputComp;
							
							TextInputCellContentComp {
								id: textInputContent;
								
								property int port: rowDelegate && rowDelegate.dataModel ? rowDelegate.dataModel.item.m_connectionParam.m_wsPort : -1
								onPortChanged: {
									reused()
								}
								
								onEditingFinished: {
									console.log("port onEditingFinished", port)
									let actualPort = serviceEditorContainer.serviceData.m_inputConnections.get(rowIndex).item.m_connectionParam.m_wsPort
									if (actualPort !== port){
										serviceEditorContainer.doUpdateModel()
									}
								}
								
								function getValue(){
									return rowDelegate.dataModel.item.m_connectionParam.m_wsPort
								}
								
								function setValue(value){
									console.log("port setValue", value)
									let urlParam = rowDelegate.dataModel.item.m_connectionParam;
									urlParam.m_wsPort = value;
									rowDelegate.dataModel.item.m_connectionParam = urlParam;
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
												values.push(item.m_url.m_host + ":" + item.m_url.m_port)
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
									font.pixelSize: Style.fontSizeM;
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
									anchors.rightMargin: Style.marginM
									
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
											let externPorts = content.rowDelegate.dataModel.item.m_externPorts
											console.log("onStarted externPorts", externPorts.toJson())
											portsModel = externPorts.copyMe()
										}
										
										onFinished: {
											if (buttonId == Enums.save){
												content.rowDelegate.dataModel.item.m_externPorts = portsModel.copyMe()
												portsModel = content.rowDelegate.dataModel.item.m_externPorts.copyMe()
												serviceEditorContainer.doUpdateModel()
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
						font.pixelSize: Style.fontSizeM;
						
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
								onEditingFinished: {
									let actual = serviceEditorContainer.serviceData.m_outputConnections.get(rowIndex).item.m_description
									if (actual != text){
										serviceEditorContainer.doUpdateModel()
									}
								}
							}
						}
						
						Component {
							id: comboBoxComp2;
							
							TableCellDelegateBase {
								id: bodyItem;
								
								property string dependantConnectionId: rowDelegate && rowDelegate.dataModel ? rowDelegate.dataModel.item.m_dependantConnectionId : ""
								onDependantConnectionIdChanged: {
									reused()
								}
								
								property var url: rowDelegate && rowDelegate.dataModel ? rowDelegate.dataModel.item.m_connectionParam : ""
								
								onReused: {
									if (rowIndex >= 0){
										let item = ouputConnTable.elements.get(bodyItem.rowIndex).item;
										
										let dependantConnectionId = item.m_dependantConnectionId;
										let elementsModel = item.m_dependantConnectionList;
										
										textLabel.text = "";
										cb.model = elementsModel;
										
										if (cb.model){
											for (let i = 0; i < cb.model.count; i++){
												if (cb.model.get(i).item.m_id == dependantConnectionId){
													cb.currentIndex = i;
													let item = cb.model.get(cb.currentIndex).item;
													textLabel.text = item.m_name
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
									font.pixelSize: Style.fontSizeM;
								}
								
								ComboBox {
									id: cb;
									
									width: parent.width;
									height: 25;
									nameId: "m_name";
									
									visible: false;
									
									onCurrentIndexChanged: {
										cb.visible = false;
										
										let dependantConnectionId = ""
										let name = ""
										let url = undefined
										
										if (currentIndex >= 0 && cb.model){
											let item = cb.model.get(cb.currentIndex).item;
											dependantConnectionId = item.m_id
											name = item.m_name
											url = item.m_connectionParam
										}
										
										textLabel.text = name;
										
										let outputItem = ouputConnTable.elements.get(bodyItem.rowIndex).item;
										outputItem.m_dependantConnectionId = dependantConnectionId;
										if (url){
											outputItem.m_connectionParam = url.copyMe();
										}
										
										if (serviceEditorContainer.serviceData.m_outputConnections.get(bodyItem.rowIndex).item.m_dependantConnectionId != dependantConnectionId){
											serviceEditorContainer.doUpdateModel()
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
								
								headersModel2.setData("id", "connectionName", index)
								headersModel2.setData("name", qsTr("Connection Name"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("id", "serviceTypeId", index)
								headersModel2.setData("name", qsTr("Service"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("id", "description", index)
								headersModel2.setData("name", qsTr("Description"), index)
								
								index = headersModel2.insertNewItem();
								
								headersModel2.setData("id", "url", index)
								headersModel2.setData("name", qsTr("Url"), index)
								
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

