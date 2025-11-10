import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import agentinoServicesSdl 1.0
import imtbaseImtBaseTypesSdl 1.0

Dialog {
	id: portsDialog;
	
	width: 500;
	
	title: qsTr("Edit ports");
	
	property BaseModel connectionListModel: BaseModel {}
	
	Component.onCompleted: {
		addButton(Enums.save, qsTr("Save"), true)
		addButton(Enums.cancel, qsTr("Cancel"), true)
	}
	
	onConnectionListModelChanged: {
		updateGui();
	}
	
	Component {
		id: externPortComp;
		ExternConnectionInfo {
		}
	}
	
	Component {
		id: urlParamComp;
		ServerConnectionParam {
			m_isSecure: false
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
		if (portsDialog.contentItem){
			portsDialog.contentItem.tableView.elements = portsDialog.connectionListModel;
		}
	}
	
	contentComp: Component{ Item {
			id: dialogBody;
			
			width: portsDialog.width;
			height: bodyColumn.height + 40;
			
			property alias tableView: tableTreeView;
			
			property TreeItemModel headersModel: TreeItemModel {
				Component.onCompleted: {
					let index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "host", index);
					dialogBody.headersModel.setData("name", qsTr("Host"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "httpPort", index);
					dialogBody.headersModel.setData("name", qsTr("Http Port"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "wsPort", index);
					dialogBody.headersModel.setData("name", qsTr("Web Socket Port"), index);
					
					index = dialogBody.headersModel.insertNewItem();
					dialogBody.headersModel.setData("id", "description", index);
					dialogBody.headersModel.setData("name", qsTr("Description"), index);
					
					tableTreeView.headers = dialogBody.headersModel;
					
					tableTreeView.elements = portsDialog.connectionListModel
				}
			}
			
			Column {
				id: bodyColumn;
				
				anchors.verticalCenter: parent.verticalCenter;
				anchors.right: parent.right;
				anchors.left: parent.left;
				anchors.rightMargin: Style.marginM;
				anchors.leftMargin: Style.marginM;
				
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
							let connectionInfo = externPortComp.createObject(portsDialog.connectionListModel);
							connectionInfo.m_id = UuidGenerator.generateUUID();
							
							let connectionParamObj = urlParamComp.createObject(connectionInfo);
							
							connectionParamObj.m_host = "localhost";
							connectionParamObj.m_httpPort = 80;
							connectionParamObj.m_wsPort = 90;
							
							connectionInfo.m_connectionParam = connectionParamObj;
							
							portsDialog.connectionListModel.addElement(connectionInfo);
							
							portsDialog.updateGui()
						}
						else if (commandId == "Remove"){
							let indexes = tableTreeView.getSelectedIndexes();
							if (indexes.length > 0){
								let index = indexes[0];
								
								portsDialog.connectionListModel.removeElement(index)
								
								tableTreeView.resetSelection();
								portsDialog.updateGui()
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
					id: wsPortInputComp;
					
					TextInputCellContentComp {
						function getValue(){
							return rowDelegate.dataModel.item.m_connectionParam.m_wsPort;
						}
					
						function setValue(value){
							let urlParam = rowDelegate.dataModel.item.m_connectionParam;
							urlParam.m_wsPort = value;
							rowDelegate.dataModel.item.m_connectionParam = urlParam;
						}
					}
				}
				
				Component {
					id: httpPortInputComp;
					
					TextInputCellContentComp {
						function getValue(){
							return rowDelegate.dataModel.item.m_connectionParam.m_httpPort;
						}
					
						function setValue(value){
							let urlParam = rowDelegate.dataModel.item.m_connectionParam;
							urlParam.m_httpPort = value;
							rowDelegate.dataModel.item.m_connectionParam = urlParam;
						}
					}
				}
				
				Component {
					id: hostInputComp;
					
					TextInputCellContentComp {
						function getValue(){
							return rowDelegate.dataModel.item.m_connectionParam.m_host;
						}
					
						function setValue(value){
							let urlParam = rowDelegate.dataModel.item.m_connectionParam;
							urlParam.m_host = value;
							rowDelegate.dataModel.item.m_connectionParam = urlParam;
						}
					}
				}
				
				Table {
					id: tableTreeView;
					
					width: parent.width;
					height: 200;
					
					radius: 0;

					onHeadersChanged: {
						tableTreeView.setColumnContentById("host", hostInputComp)
						tableTreeView.setColumnContentById("wsPort", wsPortInputComp)
						tableTreeView.setColumnContentById("httpPort", httpPortInputComp)
						tableTreeView.setColumnContentById("description", textInputComp)
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
