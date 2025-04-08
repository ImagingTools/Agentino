import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import agentinoServicesSdl 1.0

Dialog {
	id: portsDialog;
	
	width: 500;
	
	title: qsTr("Edit ports");
	
	property BaseModel portsModel: BaseModel {}
	
	Component.onCompleted: {
		addButton(Enums.save, qsTr("Save"), true)
		addButton(Enums.cancel, qsTr("Cancel"), true)

		updateGui();
	}
	
	function setPortsModel(portsModel){
		console.log("setPortsModel", portsModel.toJson())
		portsDialog.portsModel.clear();
		
		for (let i = 0; i < portsModel.count; i++){
			portsDialog.portsModel.addElement(portsModel.get(i).item.copyMe());
		}
		
		portsDialog.contentItem.tableView.elements = portsDialog.portsModel;
	}
	
	Component {
		id: externPortComp;
		ExternPort {
		}
	}
	
	Component {
		id: urlParamComp;
		UrlParameter {}
	}
	
	Keys.onPressed: {
		if (event.key === Qt.Key_Return){
			let enabled = portsDialog.buttons.getButtonState(Enums.ok);
			if (enabled){
				portsDialog.finished(Enums.save);
			}
		}
		else if (event.key === Qt.Key_Escape){
			portsDialog.finished(Enums.cancel);
		}
	}
	
	function updateGui(){
		portsDialog.contentItem.tableView.elements = portsDialog.portsModel;
	}
	
	contentComp: Component{ Item {
			id: dialogBody;
			
			width: portsDialog.width;
			height: bodyColumn.height + 40;
			
			property alias tableView: tableTreeView;
			
			property TreeItemModel headersModel: TreeItemModel {
				Component.onCompleted: {
					let index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "m_scheme", index);
					dialogBody.headersModel.setData("name", qsTr("Scheme"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "m_host", index);
					dialogBody.headersModel.setData("name", qsTr("Host"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "m_port", index);
					dialogBody.headersModel.setData("name", qsTr("Port"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "m_description", index);
					dialogBody.headersModel.setData("name", qsTr("Description"), index);
					
					tableTreeView.headers = dialogBody.headersModel;
				}
			}
			
			Column {
				id: bodyColumn;
				
				anchors.verticalCenter: parent.verticalCenter;
				anchors.right: parent.right;
				anchors.left: parent.left;
				anchors.rightMargin: Style.sizeMainMargin;
				anchors.leftMargin: Style.sizeMainMargin;
				
				width: portsDialog.width;
				
				TreeItemModel {
					id: commandsModel;
					
					Component.onCompleted: {
						let index = commandsModel.insertNewItem();
						
						commandsModel.setData("id", "Add", index);
						commandsModel.setData("name", "Add", index);
						commandsModel.setData("icon", "Icons/Add", index);
						commandsModel.setData("isEnabled", true, index);
						commandsModel.setData("visible", true, index);
						
						index = commandsModel.insertNewItem();
						
						commandsModel.setData("id", "Remove", index);
						commandsModel.setData("name", "Remove", index);
						commandsModel.setData("icon", "Icons/Delete", index);
						commandsModel.setData("isEnabled", false, index);
						commandsModel.setData("visible", true, index);
						
						commandsDecorator.commandModel = commandsModel;
					}
				}
				
				SimpleCommandsDecorator {
					id: commandsDecorator;
					
					width: parent.width;
					height: 25;
					
					onCommandActivated: {
						if (commandId == "Add"){
							let portObj = externPortComp.createObject(portsDialog.portsModel);
							portObj.m_id = UuidGenerator.generateUUID();
							portObj.m_name = "";
							portObj.m_description = "";
							
							let urlObj = urlParamComp.createObject(portObj);
							
							urlObj.m_scheme = "http";
							urlObj.m_host = "localhost";
							urlObj.m_port = 80;
							
							portObj.m_url = urlObj;
							
							portsDialog.portsModel.addElement(portObj);
						}
						else if (commandId == "Remove"){
							let indexes = tableTreeView.getSelectedIndexes();
							if (indexes.length > 0){
								let index = indexes[0];
								
								portsDialog.portsModel.removeElement(index)
								
								tableTreeView.resetSelection();
							}
						}
					}
				}
				
				Component {
					id: textInputComp;
					
					TextInputCellContentComp {
					}
				}
				
				Component {
					id: portInputComp;
					
					TextInputCellContentComp {
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
					id: hostInputComp;
					
					TextInputCellContentComp {
						function getValue(){
							return rowDelegate.dataModel.item.m_url.m_host;
						}
					
						function setValue(value){
							let urlParam = rowDelegate.dataModel.item.m_url;
							urlParam.m_host = value;
							rowDelegate.dataModel.item.m_url = urlParam;
						}
					}
				}
				
				Component {
					id: comboBoxCellContentComp;
					ComboBoxCellContentComp {
						model: schemesModel;
						
						function getValue(){
							return rowDelegate.dataModel.item.m_url.m_scheme;
						}
					
						function setValue(value){
							let urlParam = rowDelegate.dataModel.item.m_url;
							urlParam.m_scheme = value;
							rowDelegate.dataModel.item.m_url = urlParam;
						}
					}
				}
				
				TreeItemModel {
					id: schemesModel;
					
					Component.onCompleted: {
						let index = schemesModel.insertNewItem();
						schemesModel.setData("m_scheme", "http", index);
						
						index = schemesModel.insertNewItem();
						schemesModel.setData("m_scheme", "https", index);
						
						index = schemesModel.insertNewItem();
						schemesModel.setData("m_scheme", "ws", index);
						
						index = schemesModel.insertNewItem();
						schemesModel.setData("m_scheme", "wss", index);
					}
				}
				
				Table {
					id: tableTreeView;
					
					width: parent.width;
					height: 200;
					
					radius: 0;
					
					onHeadersChanged: {
						tableTreeView.setColumnContentById("m_scheme", comboBoxCellContentComp)
						tableTreeView.setColumnContentById("m_host", hostInputComp)
						tableTreeView.setColumnContentById("m_port", portInputComp)
						tableTreeView.setColumnContentById("m_description", textInputComp)
					}
					
					onSelectionChanged: {
						let isEnabled = selection.length > 0;
						
						commandsDecorator.setCommandIsEnabled("Remove", isEnabled);
					}
				}
			}
		}
	}
}
