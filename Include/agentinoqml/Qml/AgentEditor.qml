import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0

ViewBase {
    id: serviceEditorContainer;

    property int mainMargin: 0;
    property int panelWidth: 700;

    function blockEditing(){
        descriptionInput.readOnly = true;
        nameInput.readOnly = true;
    }

    function updateGui(){
        if (serviceEditorContainer.model.ContainsKey("Name")){
            nameInput.text = serviceEditorContainer.model.GetData("Name");
        }
        else{
            nameInput.text = "";
        }

        if (serviceEditorContainer.model.ContainsKey("Description")){
            descriptionInput.text = serviceEditorContainer.model.GetData("Description");
        }
        else{
            descriptionInput.text = "";
        }

        if (serviceEditorContainer.model.ContainsKey("TracingLevel")){
            let tracingLevel = serviceEditorContainer.model.GetData("TracingLevel")
            if (tracingLevel > -1){
                switchVerboseMessage.setChecked(true);
            }
            else{
                switchVerboseMessage.setChecked(false);
            }

            tracingLevelInput.currentIndex = tracingLevel
        }
        else{
            switchVerboseMessage.setChecked(false)
        }
    }

    function updateModel(){
        serviceEditorContainer.model.SetData("Name", nameInput.text);
        serviceEditorContainer.model.SetData("Description", descriptionInput.text);

        if (switchVerboseMessage.checked){
            if (tracingLevelInput.currentIndex == -1){
                tracingLevelInput.currentIndex = 0;
            }

            serviceEditorContainer.model.SetData("TracingLevel", tracingLevelInput.currentIndex);
        }
        else{
            serviceEditorContainer.model.SetData("TracingLevel", -1);
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
                    font.pixelSize: Style.fontSize_common;

                    text: qsTr("Name");
                }

                CustomTextField {
                    id: nameInput;

                    width: parent.width;
                    height: Style.itemSizeMedium;

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
                    font.pixelSize: Style.fontSize_common;

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

                    KeyNavigation.tab: nameInput;
                }

                Text {
                    id: titleVerboseMessage;

                    anchors.left: parent.left;

                    width: parent.width;

                    color: Style.textColor;
                    font.family: Style.fontFamily;
                    font.pixelSize: Style.fontSize_common;

                    text: qsTr("Verbose message (") + (switchVerboseMessage.checked ? qsTr("on") : qsTr("off")) + ")";
                }

                Row {
                    height:  Style.itemSizeMedium;
                    spacing: Style.size_mainMargin;

                    SwitchCustom {
                        id: switchVerboseMessage
                        anchors.verticalCenter: parent.verticalCenter

                        backgroundColor: "#D4D4D4"
                        onCheckedChanged: {
                            serviceEditorContainer.doUpdateModel();
                        }
                    }

                    Item {
                        width: Style.size_mainMargin;
                        height: Style.itemSizeMedium
                    }

                    Text {
                        id: titleTracingLevel;
                        anchors.verticalCenter: parent.verticalCenter

                        visible: switchVerboseMessage.checked
                        color: Style.textColor;
                        font.family: Style.fontFamily;
                        font.pixelSize: Style.fontSize_common;

                        text: qsTr("Tracing level");
                    }

                    ComboBox {
                        id: tracingLevelInput
                        anchors.verticalCenter: parent.verticalCenter
                        height: Style.itemSizeMedium * 0.75;
                        width: Style.itemSizeLarge;
                        visible: switchVerboseMessage.checked

                        model: TreeItemModel {}

                        Component.onCompleted: {
                            let index = model.InsertNewItem()
                            model.SetData("Name", "0", index)

                            index = model.InsertNewItem()
                            model.SetData("Name", "1", index)

                            index = model.InsertNewItem()
                            model.SetData("Name", "2", index)

                            index = model.InsertNewItem()
                            model.SetData("Name", "3", index)

                            index = model.InsertNewItem()
                            model.SetData("Name", "4", index)

                            index = model.InsertNewItem()
                            model.SetData("Name", "5", index)
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
