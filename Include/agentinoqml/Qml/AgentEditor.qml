import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcontrols 1.0
import agentinoAgentsSdl 1.0

ViewBase {
	id: serviceEditorContainer;
	
	property int mainMargin: 0;
	property int panelWidth: 700;
	
	property AgentData agentData: model ? model : null;
	
	function updateGui(){
		nameInput.text = agentData.m_name
		descriptionInput.text = agentData.m_description
		
		switchVerboseMessage.checked = (agentData.m_tracingLevel > -1);
		
		if (agentData.m_tracingLevel > -1){
			tracingLevelInput.currentIndex = agentData.m_tracingLevel;
		}
		else{
			tracingLevelInput.currentIndex = -1;
		}
	}
	
	function updateModel(){
		agentData.m_name = nameInput.text;
		agentData.m_description = descriptionInput.text;
		
		if (switchVerboseMessage.checked){
			if (tracingLevelInput.currentIndex < 0){
				agentData.m_tracingLevel = 0;
			}
			else{
				agentData.m_tracingLevel = tracingLevelInput.currentIndex;
			}
		}
		else{
			agentData.m_tracingLevel = -1;
		}
	}	
	Rectangle {
		id: background;
		
		anchors.fill: parent;
		
		color: Style.backgroundColor;
		
		Item {
			id: columnContainer;
			
			width: serviceEditorContainer.panelWidth;
			height: bodyColumn.height + 2 * bodyColumn.anchors.topMargin;
			
			Column {
				id: bodyColumn;
				
				anchors.top: parent.top;
				anchors.left: parent.left;
				anchors.topMargin: serviceEditorContainer.mainMargin;
				anchors.leftMargin: serviceEditorContainer.mainMargin;
				
				width: serviceEditorContainer.panelWidth - 2*anchors.leftMargin;
				
				spacing: 10;
				
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
					
					KeyNavigation.tab: nameInput;
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
						
						backgroundColor: "#D4D4D4"
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
						
						model: TreeItemModel {}
						
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
			}//Column bodyColumn
		}//columnContainer
	}
}//Container
