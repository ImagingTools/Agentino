import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0

ViewBase {
    id: serviceEditorContainer;

    property int radius: 3;
    property int flickableWidth: 800;

    Component.onCompleted: {
        let ok = PermissionsController.checkPermission("ChangeService");

        serviceEditorContainer.readOnly = !ok;
    }

    onWidthChanged: {
        console.log("onWidthChanged", width);
        if (width < flickableWidth + 50){
            flickable.width = width - 50;
        }
        else{
            flickable.width = flickableWidth;
        }
    }

    function setReadOnly(readOnly){
        nameInput.readOnly = readOnly;
        pathInput.readOnly = readOnly;
        argumentsInput.readOnly = readOnly;
        descriptionInput.readOnly = readOnly;

        inputConnTable.readOnly = readOnly;
        ouputConnTable.readOnly = readOnly;

        switchAutoStart.enabled = !readOnly;
        switchVerboseMessage.enabled = !readOnly
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

        if (serviceEditorContainer.model.ContainsKey("StartScript")){
            let startScript = serviceEditorContainer.model.GetData("StartScript")
            if (startScript !== ""){
                startScriptChecked.checkState = Qt.Checked
                startScriptInput.text = startScript
            }
            else{
                startScriptChecked.checkState = Qt.Unchecked
                startScriptInput.text = ""
            }
        }
        else{
            startScriptChecked.checkState = Qt.Unchecked
            startScriptInput.text = ""
        }

        if (serviceEditorContainer.model.ContainsKey("StopScript")){
            let stopScript = serviceEditorContainer.model.GetData("StopScript")
            if (stopScript !== ""){
                stopScriptChecked.checkState = Qt.Checked
                stopScriptInput.text = stopScript
            }
            else{
                stopScriptChecked.checkState = Qt.Unchecked
            }
        }
        else{
            stopScriptChecked.checkState = Qt.Unchecked
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
        serviceEditorContainer.model.SetData("Arguments", argumentsInput.text);
        serviceEditorContainer.model.SetData("IsAutoStart", switchAutoStart.checked);
        serviceEditorContainer.model.SetData("StartScript", startScriptInput.text);
        serviceEditorContainer.model.SetData("StopScript", stopScriptInput.text);

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

        width: serviceEditorContainer.flickableWidth;

        contentWidth: bodyColumn.width;
        contentHeight: bodyColumn.height;

        boundsBehavior: Flickable.StopAtBounds;

        clip: true;

        Column {
            id: bodyColumn;

            width: flickable.width;

            spacing: Style.size_mainMargin;

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

                KeyNavigation.tab: pathInput;
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

                KeyNavigation.tab: argumentsInput;
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

                KeyNavigation.tab: nameInput;
            }

            Text {
                id: titleAutoStart;

                anchors.left: parent.left;

                width: parent.width;

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

                    model: TreeItemModel {
                    }
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

            CheckBox {
                id: startScriptChecked
                text: qsTr("Start script")
                onClicked: {
                    if(startScriptChecked.checkState !== Qt.Checked){
                        startScriptChecked.checkState = Qt.Checked;
                    }
                    else {
                        startScriptChecked.checkState = Qt.Unchecked;
                    }
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
                onClicked: {
                    if(stopScriptChecked.checkState !== Qt.Checked){
                        stopScriptChecked.checkState = Qt.Checked;
                    }
                    else {
                        stopScriptChecked.checkState = Qt.Unchecked;
                    }
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
                font.pixelSize: Style.fontSize_common;

                text: qsTr("Incoming Connections");

                visible: false;
            }

            AuxTable {
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
                    let count = elements.GetItemsCount();

                    inputConnectionsTitle.visible = count > 0;
                    inputConnTable.visible = count > 0;
                }

                onHeadersChanged: {
                    inputConnTable.setColumnContentComponent(1, textInputComp)
                    inputConnTable.setColumnContentComponent(3, textInputComp)
                    inputConnTable.setColumnContentComponent(4, externCompEditComp)

                    inputConnTable.tableDecorator = inputTableDecoratorModel;
                }

                onSelectionChanged: {
                    ouputConnTable.resetSelection();
                }

                TreeItemModel {
                    id: headersModel;

                    Component.onCompleted: {
                        let index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "ConnectionName", index)
                        headersModel.SetData("Name", qsTr("Usage"), index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Description", index)
                        headersModel.SetData("Name", qsTr("Description"), index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Host", index)
                        headersModel.SetData("Name", qsTr("Host"), index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "Port", index)
                        headersModel.SetData("Name", qsTr("Port"), index)

                        index = headersModel.InsertNewItem();

                        headersModel.SetData("Id", "ExternPorts", index)
                        headersModel.SetData("Name", qsTr("Extern Addresses"), index)

                        inputConnTable.headers = headersModel;
                    }
                }

                TreeItemModel {
                    id: inputTableDecoratorModel;

                    Component.onCompleted: {
                        var cellWidthModel = inputTableDecoratorModel.AddTreeModel("CellWidth");

                        let index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 150, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 300, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 100, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 100, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", -1, index);
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
                            if (tableCellDelegate){
                                let valueModel = tableCellDelegate.getValue();
                                if (valueModel){
                                    let values = []
                                    for (let i = 0; i < valueModel.GetItemsCount(); i++){
                                        let port = valueModel.GetData("Port", i);
                                        let host = valueModel.GetData("Host", i)

                                        values.push(host + ":" + port)
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
                            font.pixelSize: Style.fontSize_common;
                            lineHeight: 1.5;

                            onTextChanged: {
                                if (content.tableCellDelegate){
                                    console.log("onTextChanged", text);
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

                            width: 18;
                            height: width;

                            iconSource: "../../../../" + Style.getIconPath("Icons/Edit", Icon.State.Off, Icon.Mode.Normal);

                            visible: !serviceEditorContainer.readOnly;

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
                font.pixelSize: Style.fontSize_common;

                text: qsTr("Dependant Services");

                visible: false;
            }

            AuxTable {
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
                    let count = elements.GetItemsCount();

                    dependantServicesTitle.visible = count > 0;
                    ouputConnTable.visible = count > 0;
                }

                TextInputCellContentComp {
                    id: textInputComp2;
                }

                Component {
                    id: comboBoxComp2;

                    Item {
                        id: bodyItem;

                        property Item tableCellDelegate: null;

//                        z: parent.z + 1;

                        width: parent.width;
                        height: 25;

                        property bool ok: tableCellDelegate != null && ouputConnTable.elements;

                        onOkChanged: {
                            if (tableCellDelegate){
                                let value = tableCellDelegate.getValue();

                                let dependantConnectionId = ouputConnTable.elements.GetData("DependantConnectionId", bodyItem.tableCellDelegate.rowIndex);

                                let elementsModel = ouputConnTable.elements.GetData("Elements", tableCellDelegate.rowIndex);
                                textLabel.text = value;
                                cb.model = elementsModel;

                                if (cb.model){
                                    for (let i = 0; i < cb.model.GetItemsCount(); i++){
                                        let id = cb.model.GetData("Id", i);
                                        if (String(id) == dependantConnectionId){
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
                                        let id = cb.model.GetData("Id", cb.currentIndex)
                                        let name = cb.model.GetData("Name", cb.currentIndex)
                                        let url = cb.model.GetData("Url", cb.currentIndex)

                                        textLabel.text = name;

                                        ouputConnTable.elements.SetData("DependantConnectionId", id, bodyItem.tableCellDelegate.rowIndex);
                                        ouputConnTable.elements.SetData("Url", url, bodyItem.tableCellDelegate.rowIndex);

//                                        bodyItem.tableCellDelegate.setValue(name);
                                    }
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
                                if (cb.model && cb.model.GetItemsCount() > 0){
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
                        let index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "ConnectionName", index)
                        headersModel2.SetData("Name", qsTr("Usage"), index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "ServiceTypeName", index)
                        headersModel2.SetData("Name", qsTr("Service"), index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "Description", index)
                        headersModel2.SetData("Name", qsTr("Description"), index)

                        index = headersModel2.InsertNewItem();

                        headersModel2.SetData("Id", "DisplayUrl", index)
                        headersModel2.SetData("Name", qsTr("Url"), index)

                        ouputConnTable.headers = headersModel2;
                    }
                }

                TreeItemModel {
                    id: outputTableDecoratorModel;

                    Component.onCompleted: {
                        var cellWidthModel = outputTableDecoratorModel.AddTreeModel("CellWidth");

                        let index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 150, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 80, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", 350, index);

                        index = cellWidthModel.InsertNewItem();
                        cellWidthModel.SetData("Width", -1, index);
                    }
                }
            }
        }//Column bodyColumn
    }
}//Container
