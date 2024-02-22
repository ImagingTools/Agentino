import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0

ViewBase {
    id: serviceEditorContainer;

    property int mainMargin: 0;
    property int panelWidth: 700;
    property int radius: 3;

    function blockEditing(){
        pathInput.readOnly = true;
        nameInput.readOnly = true;
        settingsPathInput.readOnly = true;
        argumentsInput.readOnly = true;
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

        if (serviceEditorContainer.model.ContainsKey("Path")){
            pathInput.text = serviceEditorContainer.model.GetData("Path");
        }
        else{
            pathInput.text = "";
        }

        if (serviceEditorContainer.model.ContainsKey("SettingsPath")){
            settingsPathInput.text = serviceEditorContainer.model.GetData("SettingsPath");
        }
        else{
            settingsPathInput.text = "";
        }

        if (serviceEditorContainer.model.ContainsKey("Arguments")){
            argumentsInput.text = serviceEditorContainer.model.GetData("Arguments");
        }
        else{
            argumentsInput.text = "";
        }

        if (serviceEditorContainer.model.ContainsKey("IsAutoStart")){
            switchAutoStart.setChecked(serviceEditorContainer.model.GetData("IsAutoStart"));
        }
        else{
            switchAutoStart.setChecked(false)
        }

        if (serviceEditorContainer.model.ContainsKey("InputConnections")){
            inputConnTable.elements = serviceEditorContainer.model.GetData("InputConnections")
        }

        if (serviceEditorContainer.model.ContainsKey("OutputConnections")){
            ouputConnTable.elements = serviceEditorContainer.model.GetData("OutputConnections")
        }
    }

    function updateModel(){
        serviceEditorContainer.model.SetData("Name", nameInput.text);
        serviceEditorContainer.model.SetData("Description", descriptionInput.text);
        serviceEditorContainer.model.SetData("Path", pathInput.text);
        serviceEditorContainer.model.SetData("SettingsPath", settingsPathInput.text);
        serviceEditorContainer.model.SetData("Arguments", argumentsInput.text);
        serviceEditorContainer.model.SetData("IsAutoStart", switchAutoStart.checked);
    }

    Rectangle {
        id: background;

        anchors.fill: flickable;

        color: Style.backgroundColor;
    }

    MouseArea {
        anchors.fill: background;

        onClicked: {
            serviceEditorContainer.forceActiveFocus();
        }
    }

    CustomScrollbar {
        id: scrollbar;

        anchors.left: flickable.right;
        anchors.leftMargin: 5;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        secondSize: 10;
        targetItem: flickable;

        visible: parent.visible;
    }

    Flickable {
        id: flickable;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.leftMargin: 20;

        width: 700;

        contentWidth: bodyColumn.width;
        contentHeight: bodyColumn.height;

        boundsBehavior: Flickable.StopAtBounds;

        clip: true;

        Column {
            id: bodyColumn;

            width: flickable.width;

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

                KeyNavigation.tab: settingsPathInput;
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

                placeHolderText: qsTr("Enter the name");

                onEditingFinished: {
                    serviceEditorContainer.doUpdateModel();
                }

                KeyNavigation.tab: settingsPathInput;
            }

            Text {
                id: titlePath;

                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

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

                KeyNavigation.tab: settingsPathInput;
            }

            Text {
                id: titleSettingsPath;

                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

                text: qsTr("Settings Path");
            }

            CustomTextField {
                id: settingsPathInput;

                width: parent.width;
                height: Style.itemSizeMedium;

                placeHolderText: qsTr("Enter the settings path");

                onEditingFinished: {
                    serviceEditorContainer.doUpdateModel();
                }

                KeyNavigation.tab: settingsPathInput;
            }

            Text {
                id: titleArguments;

                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

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

                KeyNavigation.tab: settingsPathInput;
            }

            Text {
                id: titleAutoStart;

                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

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
                id: inputConnectionsTitle;

                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

                text: qsTr("Incoming Connections");
            }

            AuxTable {
                id: inputConnTable;

                width: parent.width;
                height: 150;

                onHeadersChanged: {
                    inputConnTable.setColumnContentComponent(1, textInputComp)
                    inputConnTable.setColumnContentComponent(3, textInputComp)
                    inputConnTable.setColumnContentComponent(4, externCompEditComp)
                }

                onSelectionChanged: {
                    ouputConnTable.resetSelection();
                }

                TreeItemModel {
                    id: headersModel;

                    Component.onCompleted: {
                        let index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "ConnectionName", index)
                        headersModel.SetData("Name", "Connection Name", index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Description", index)
                        headersModel.SetData("Name", "Description", index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Host", index)
                        headersModel.SetData("Name", "Host", index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Port", index)
                        headersModel.SetData("Name", "Port", index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "ExternPorts", index)
                        headersModel.SetData("Name", "Extern Ports", index)

                        inputConnTable.headers = headersModel;
                    }
                }

                TextInputCellContentComp {
                    id: textInputComp;
                }

                Component {
                    id: externCompEditComp;
                    Item {
                        id: content;
                        width: parent.width;

                        property Item tableCellDelegate: null;

                        onTableCellDelegateChanged: {
                            if (tableCellDelegate.mainMouseArea){
                                tableCellDelegate.mainMouseArea.hoverEnabled = false;

                                let valueModel = tableCellDelegate.getValue();
                                if (valueModel){
                                    let values = []
                                    for (let i = 0; i < valueModel.GetItemsCount(); i++){
                                        let port = valueModel.GetData("Port", i);
                                        let host = valueModel.GetData("Host", i)

                                        values.push(host + ":" + port)
                                    }

                                    textLabel.text = values.join(';')
                                }
                            }
                        }

                        Text {
                            id: textLabel;

                            anchors.verticalCenter: parent.verticalCenter;
                            anchors.left: parent.left;
                            anchors.right: button.left;

                            elide: Text.ElideRight;
                            wrapMode: Text.NoWrap;

                            clip: true;

                            color: Style.textColor;
                            font.family: Style.fontFamily;
                            font.pixelSize: Style.fontSize_common;
                        }

                        ToolButton {
                            id: button;

                            anchors.verticalCenter: parent.verticalCenter;
                            anchors.right: parent.right;

                            width: 18;
                            height: width;

                            iconSource: "../../../../" + Style.getIconPath("Icons/Edit", Icon.State.Off, Icon.Mode.Normal);

                            onClicked: {
                                if (inputConnTable.elements.ContainsKey("ExternPorts", content.tableCellDelegate.rowIndex)){
                                    let externPortsModel = inputConnTable.elements.GetData("ExternPorts", content.tableCellDelegate.rowIndex);
                                    if (externPortsModel){
                                        modalDialogManager.openDialog(externPortsDialogComp, {"portsModel": externPortsModel.CopyMe()});
                                    }
                                }
                            }
                        }

                        Component {
                            id: externPortsDialogComp;

                            ExternPortsDialog {
                                onFinished: {
                                    if (buttonId == Enums.save){
                                        if (content.tableCellDelegate.rowIndex >= 0){
                                            let ports = []
                                            for (let i = 0; i < portsModel.GetItemsCount(); i++){
                                                let port = portsModel.GetData("Port", i);
                                                let host = portsModel.GetData("Host", i);
                                                ports.push(host + ":" + port)
                                            }

                                            let externPortsModel = inputConnTable.elements.GetData("ExternPorts", content.tableCellDelegate.rowIndex);

                                            externPortsModel.Copy(portsModel);
                                            externPortsModel.Refresh()

                                            textLabel.text = ports.join(';');
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                anchors.left: parent.left;

                color: Style.textColor;
                font.family: Style.fontFamily;
                font.pixelSize: Style.fontSize_common;

                text: qsTr("Dependant Services");
            }

            AuxTable {
                id: ouputConnTable;

                width: parent.width;
                height: 150;

                onHeadersChanged: {
                    ouputConnTable.setColumnContentComponent(2, textInputComp2)
                    ouputConnTable.setColumnContentComponent(3, comboBoxComp2)
                }

                TextInputCellContentComp {
                    id: textInputComp2;
                }

                Component {
                    id: comboBoxComp2;

                    Item {
                        id: bodyItem;

                        property Item tableCellDelegate: null;

                        z: parent.z + 1;

                        width: parent.width;
                        height: 25;

                        property bool ok: tableCellDelegate != null && ouputConnTable.elements;

                        onOkChanged: {
                            console.log("onTableCellDelegateChanged onOkChanged")

                            if (tableCellDelegate){
                                let value = tableCellDelegate.getValue();
                                console.log("rowIndex", tableCellDelegate.rowIndex)

                                let elementsModel = ouputConnTable.elements.GetData("Elements", tableCellDelegate.rowIndex);
                                console.log("elementsModel", elementsModel)

                                textLabel.text = value;

                                cb.model = elementsModel;
                                if (cb.model){
                                    for (let i = 0; i < cb.model.GetItemsCount(); i++){
                                        let id = cb.model.GetData("Id", i);
                                        if (id == value){
                                            cb.currentIndex = i;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        onTableCellDelegateChanged: {
                            console.log("onTableCellDelegateChanged")
                        }

                        Text {
                            id: textLabel;

                            anchors.verticalCenter: parent.verticalCenter;

                            width: parent.width;

                            color: Style.textColor;
                            font.family: Style.fontFamily;
                            font.pixelSize: Style.fontSize_common;
                        }

                        ComboBox {
                            id: cb;

                            width: parent.width;
                            height: 25;

                            visible: false;

                            onCurrentIndexChanged: {
                                cb.visible = false;

                                if (bodyItem.tableCellDelegate){
                                    if (cb.model){
                                        let name = cb.model.GetData("Name", cb.currentIndex)

                                        textLabel.text = name;

//                                        let valueModel = bodyItem.tableCellDelegate.getValue();
//                                        valueModel.SetData("CurrentIndex", cb.currentIndex);

                                        bodyItem.tableCellDelegate.setValue(name);
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent;

                            onDoubleClicked: {
                                cb.visible = true;

                                cb.forceActiveFocus();
                            }
                        }
                    }
                }

//                ComboBoxCellContentComp {
//                    id: comboBoxComp2;
//                }

                TreeItemModel {
                    id: headersModel2;

                    Component.onCompleted: {
                        let index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "ConnectionName", index)
                        headersModel2.SetData("Name", "Connection Name", index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "ServiceName", index)
                        headersModel2.SetData("Name", "Service Name", index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "Description", index)
                        headersModel2.SetData("Name", "Description", index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "Url", index)
                        headersModel2.SetData("Name", "Url", index)

                        ouputConnTable.headers = headersModel2;
                    }
                }
            }
        }//Column bodyColumn
    }
}//Container
