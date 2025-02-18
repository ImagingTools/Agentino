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
		buttonsModel.append({Id: Enums.save, Name:qsTr("Save"), Enabled: true})
		buttonsModel.append({Id: Enums.cancel, Name:qsTr("Cancel"), Enabled: true})
		
		updateGui();
	}
	
	function setPortsModel(portsModel){
		portsDialog.portsModel.clear();
		
		for (let i = 0; i < portsModel.count; i++){
			portsDialog.portsModel.append(portsModel.get(i));
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
		UrlParam {
		}
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
					dialogBody.headersModel.setData("Id", "m_scheme", index);
					dialogBody.headersModel.setData("Name", qsTr("Scheme"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("Id", "m_host", index);
					dialogBody.headersModel.setData("Name", qsTr("Host"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("Id", "m_port", index);
					dialogBody.headersModel.setData("Name", qsTr("Port"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("Id", "m_description", index);
					dialogBody.headersModel.setData("Name", qsTr("Description"), index);
					
					tableTreeView.headers = dialogBody.headersModel;
				}
			}
			
			Column {
				id: bodyColumn;
				
				anchors.verticalCenter: parent.verticalCenter;
				anchors.right: parent.right;
				anchors.left: parent.left;
				anchors.rightMargin: Style.size_mainMargin;
				anchors.leftMargin: Style.size_mainMargin;
				
				width: portsDialog.width;
				
				TreeItemModel {
					id: commandsModel;
					
					Component.onCompleted: {
						let index = commandsModel.insertNewItem();
						
						commandsModel.setData("Id", "Add", index);
						commandsModel.setData("Name", "Add", index);
						commandsModel.setData("Icon", "Icons/Add", index);
						commandsModel.setData("IsEnabled", true, index);
						commandsModel.setData("Visible", true, index);
						
						index = commandsModel.insertNewItem();
						
						commandsModel.setData("Id", "Remove", index);
						commandsModel.setData("Name", "Remove", index);
						commandsModel.setData("Icon", "Icons/Delete", index);
						commandsModel.setData("IsEnabled", false, index);
						commandsModel.setData("Visible", true, index);
						
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
