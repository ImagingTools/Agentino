import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0

ViewBase {
    id: serviceEditorContainer;

    property int radius: 3;
    property int flickableWidth: 800;
    property string productId;
    property string agentId;
    property var documentManager: null;

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

    onModelChanged: {
        if (model){
            serviceEditorContainer.productId = model.getData("Name");
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

    function getAdditionalInputParams(){
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
            contentHeight: bodyContainer.height;

            boundsBehavior: Flickable.StopAtBounds;

            clip: true;

            function updateGui(){
                if (serviceEditorContainer.model.containsKey("Name")){
                    nameInput.text = serviceEditorContainer.model.getData("Name");
                }
                else{
                    nameInput.text = "";
                }

                if (serviceEditorContainer.model.containsKey("Description")){
                    descriptionInput.text = serviceEditorContainer.model.getData("Description");
                }
                else{
                    descriptionInput.text = "";
                }

                if (serviceEditorContainer.model.containsKey("Path")){
                    pathInput.text = serviceEditorContainer.model.getData("Path");
                }
                else{
                    pathInput.text = "";
                }

                if (serviceEditorContainer.model.containsKey("Arguments")){
                    argumentsInput.text = serviceEditorContainer.model.getData("Arguments");
                }
                else{
                    argumentsInput.text = "";
                }

                if (serviceEditorContainer.model.containsKey("IsAutoStart")){
                    switchAutoStart.setChecked(serviceEditorContainer.model.getData("IsAutoStart"));
                }
                else{
                    switchAutoStart.setChecked(false)
                }

                if (serviceEditorContainer.model.containsKey("TracingLevel")){
                    let tracingLevel = serviceEditorContainer.model.getData("TracingLevel")
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

                if (serviceEditorContainer.model.containsKey("StartScript")){
                    let startScript = serviceEditorContainer.model.getData("StartScript")
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

                if (serviceEditorContainer.model.containsKey("StopScript")){
                    let stopScript = serviceEditorContainer.model.getData("StopScript")
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

                if (serviceEditorContainer.model.containsKey("InputConnections")){
                    inputConnTable.elements = serviceEditorContainer.model.getData("InputConnections")
                }

                if (serviceEditorContainer.model.containsKey("OutputConnections")){
                    ouputConnTable.elements = serviceEditorContainer.model.getData("OutputConnections")
                }
            }

            function updateModel(){
                serviceEditorContainer.model.setData("Name", nameInput.text);
                serviceEditorContainer.model.setData("Description", descriptionInput.text);
                serviceEditorContainer.model.setData("Path", pathInput.text);
                serviceEditorContainer.model.setData("Arguments", argumentsInput.text);
                serviceEditorContainer.model.setData("IsAutoStart", switchAutoStart.checked);

                if (switchVerboseMessage.checked){
                    if (tracingLevelInput.currentIndex == -1){
                        tracingLevelInput.currentIndex = 0;
                    }

                    serviceEditorContainer.model.setData("TracingLevel", tracingLevelInput.currentIndex);
                }
                else{
                    serviceEditorContainer.model.setData("TracingLevel", -1);
                }

                if(startScriptChecked.checkState === Qt.Checked){
                    serviceEditorContainer.model.setData("StartScript", startScriptInput.text);
                }
                else{
                    serviceEditorContainer.model.setData("StartScript", "");
                }

                if(stopScriptChecked.checkState === Qt.Checked){
                    serviceEditorContainer.model.setData("StopScript", stopScriptInput.text);
                }
                else{
                    serviceEditorContainer.model.setData("StopScript", "");
                }
            }

            CustomScrollbar {
                id: scrollbar;

                anchors.left: flickable.right;
                anchors.leftMargin: Style.size_smallMargin;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;

                secondSize: Style.size_mainMargin;
                targetItem: flickable;

                visible: parent.visible;
            }

            Item {
                id: bodyContainer
                width: flickable.width
                height: bodyColumn.height + Style.size_largeMargin * 2

            Column {
                id: bodyColumn;

                anchors.top: bodyContainer.top
                anchors.left: bodyContainer.left;
                anchors.right: bodyContainer.right
                anchors.margins: Style.size_largeMargin

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
                    onClicked: {
                        if(startScriptChecked.checkState !== Qt.Checked){
                            startScriptChecked.checkState = Qt.Checked;
                        }
                        else{
                            startScriptChecked.checkState = Qt.Unchecked;
                        }

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
                    onClicked: {
                        if(stopScriptChecked.checkState !== Qt.Checked){
                            stopScriptChecked.checkState = Qt.Checked;
                        }
                        else {
                            stopScriptChecked.checkState = Qt.Unchecked;
                        }

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
                    font.pixelSize: Style.fontSize_common;

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
                            let index = headersModel.insertNewItem();

                            headersModel.setData("Id", "ConnectionName", index)
                            headersModel.setData("Name", qsTr("Usage"), index)

                            index = headersModel.insertNewItem();

                            headersModel.setData("Id", "Description", index)
                            headersModel.setData("Name", qsTr("Description"), index)

                            index = headersModel.insertNewItem();

                            headersModel.setData("Id", "Host", index)
                            headersModel.setData("Name", qsTr("Host"), index)

                            index = headersModel.insertNewItem();

                            headersModel.setData("Id", "Port", index)
                            headersModel.setData("Name", qsTr("Port"), index)

                            index = headersModel.insertNewItem();

                            headersModel.setData("Id", "ExternPorts", index)
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
                            cellWidthModel.setData("Width", 100, index);

                            index = cellWidthModel.insertNewItem();
                            cellWidthModel.setData("Width", -1, index);
                        }
                    }

                    TextInputCellContentComp {
                        id: textInputComp;
                    }

                    Component {
                        id: externCompEditComp;
                        TableCellDelegateBase {
                            id: content;

                            onRowIndexChanged: {
                                console.log("Service editor width", width, rowIndex)
                                if (rowIndex >= 0){

                                    let valueModel = getValue();
                                    if (valueModel){
                                        let values = []
                                        for (let i = 0; i < valueModel.getItemsCount(); i++){
                                            let port = valueModel.getData("Port", i);
                                            let host = valueModel.getData("Host", i)
                                            values.push(host + ":" + port)
                                        }
                                        textLabel.text = values.join('\n')
                                    }
                                }
                            }

                            onWidthChanged: {
                                console.log("Service editor onWidthChanged", width, rowIndex)
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
                                anchors.rightMargin: Style.size_mainMargin

                                width: 18;
                                height: width;

                                iconSource: "../../../../" + Style.getIconPath("Icons/Edit", Icon.State.Off, Icon.Mode.Normal);

                                visible: !serviceEditorContainer.readOnly;

                                onClicked: {
                                    console.log("Edit onClicked")
                                    if (inputConnTable.elements.containsKey("ExternPorts", content.rowIndex)){
                                        let externPortsModel = inputConnTable.elements.getData("ExternPorts", content.rowIndex);
                                        if (externPortsModel){
                                            ModalDialogManager.openDialog(externPortsDialogComp, {"portsModel": externPortsModel.copyMe()});
                                        }
                                    }
                                }
                            }

                            Component {
                                id: externPortsDialogComp;

                                ExternPortsDialog {
                                    onFinished: {
                                        if (buttonId == Enums.save){
                                            if (content.rowIndex >= 0){
                                                let ports = []
                                                for (let i = 0; i < portsModel.getItemsCount(); i++){
                                                    let port = portsModel.getData("Port", i);
                                                    let host = portsModel.getData("Host", i);
                                                    ports.push(host + ":" + port)
                                                }

                                                let externPortsModel = inputConnTable.elements.getData("ExternPorts", content.rowIndex);

                                                externPortsModel.copy(portsModel);
                                                externPortsModel.refresh()

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

                    TextInputCellContentComp {
                        id: textInputComp2;
                    }

                    Component {
                        id: comboBoxComp2;

                        TableCellDelegateBase {
                            id: bodyItem;

                            onRowIndexChanged: {
                                if (rowIndex >= 0){

                                    let value = getValue();

                                    let dependantConnectionId = ouputConnTable.elements.getData("DependantConnectionId", bodyItem.rowIndex);

                                    let elementsModel = ouputConnTable.elements.getData("Elements", bodyItem.rowIndex);
                                    textLabel.text = value;
                                    cb.model = elementsModel;

                                    if (cb.model){
                                        for (let i = 0; i < cb.model.getItemsCount(); i++){
                                            let id = cb.model.getData("Id", i);
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
                                    if (cb.model){
                                        let id = cb.model.getData("Id", cb.currentIndex)
                                        let name = cb.model.getData("Name", cb.currentIndex)
                                        let url = cb.model.getData("Url", cb.currentIndex)

                                        textLabel.text = name;

                                        ouputConnTable.elements.setData("DependantConnectionId", id, bodyItem.rowIndex);
                                        ouputConnTable.elements.setData("Url", url, bodyItem.rowIndex);

                                        //                                        bodyItem.tableCellDelegate.setValue(name);
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
                                    if (cb.model && cb.model.getItemsCount() > 0){
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

                            headersModel2.setData("Id", "ConnectionName", index)
                            headersModel2.setData("Name", qsTr("Usage"), index)

                            index = headersModel2.insertNewItem();

                            headersModel2.setData("Id", "ServiceTypeName", index)
                            headersModel2.setData("Name", qsTr("Service"), index)

                            index = headersModel2.insertNewItem();

                            headersModel2.setData("Id", "Description", index)
                            headersModel2.setData("Name", qsTr("Description"), index)

                            index = headersModel2.insertNewItem();

                            headersModel2.setData("Id", "DisplayUrl", index)
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

            function getAdditionalInputParams(){
                if (serviceEditorContainer.productId === ""){
                    console.error("Unable to get additional parameters. Product-ID is empty");
                    return null;
                }

                let obj = serviceEditorContainer.getAdditionalInputParams();
                obj["ProductId"] = serviceEditorContainer.productId;
                obj["token"] = userTokenProvider.token;

                return obj;
            }

            UserTokenProvider {
                id: userTokenProvider
                productId: serviceEditorContainer.productId;
                isTokenGlobal: false

                function getAdditionalInputParams(){
                    return administrationViewItem.getAdditionalInputParams();
                }

                onAccepted: {
                    authorizationPage.visible = false;
                    loader.sourceComponent = administrationView
                }

                onFailed: {
                    root.loginFailed();
                }
            }

            AuthorizationPage {
                id: authorizationPage
                anchors.fill: parent;
                function onLogin(login, password){
                    userTokenProvider.authorization(login, password)
                }
            }

            Loader {
                id: loader
                anchors.fill: parent;
            }

            Component {
                id: administrationView;
                AdministrationView {
                    anchors.fill: parent;
                    productId: serviceEditorContainer.productId;
                    documentManager: serviceEditorContainer.documentManager;

                    function getAdditionalInputParams(){
                        return administrationViewItem.getAdditionalInputParams();
                    }
                }
            }
        }
    }
}//serviceEditorContainer

