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
        console.log("ServiceEditor updateGui");

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
            switchAutoStart.checked = serviceEditorContainer.model.GetData("IsAutoStart");
        }
        else{
            switchAutoStart.checked = false;
        }
    }

    function updateModel(){
        console.log("ServiceEditor updateModel");

        serviceEditorContainer.model.SetData("Name", nameInput.text);
        serviceEditorContainer.model.SetData("Description", descriptionInput.text);
        serviceEditorContainer.model.SetData("Path", pathInput.text);
        serviceEditorContainer.model.SetData("SettingsPath", settingsPathInput.text);
        serviceEditorContainer.model.SetData("Arguments", argumentsInput.text);
        serviceEditorContainer.model.SetData("IsAutoStart", switchAutoStart.checked);
    }

    Component{
        id: emptyDecorator;
        Item{
            property Item rootItem: null;
        }
    }

    Rectangle {
        id: background;

        anchors.fill: parent;

        color: Style.backgroundColor;
        Loader{
            id: backgroundDecoratorLoader;

            sourceComponent: Style.backGroundDecorator !==undefined ? Style.backGroundDecorator: emptyDecorator;
            onLoaded: {
                if(backgroundDecoratorLoader.item){
                    backgroundDecoratorLoader.item.rootItem = background;
                }
            }
        }

        //
        Item{
            id: columnContainer;

            width: serviceEditorContainer.panelWidth;
            height: bodyColumn.height + 2*bodyColumn.anchors.topMargin;
            Loader{
                id: mainPanelFrameLoader;

                anchors.fill: parent;

                sourceComponent: Style.frame !==undefined ? Style.frame: emptyDecorator;

                onLoaded: {
                    if(mainPanelFrameLoader.item){
                        // serviceEditorContainer.mainMargin = mainPanelFrameLoader.item.mainMargin;
                    }
                }
            }//Loader

            Column {
                id: bodyColumn;

                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.topMargin: serviceEditorContainer.mainMargin;
                anchors.leftMargin: serviceEditorContainer.mainMargin;

                width: serviceEditorContainer.panelWidth - 2*anchors.leftMargin;

                spacing: 10;

                Item{
                    width: parent.width;
                    height: 1;
                }


                Text {
                    id: titleName;

                    anchors.left: parent.left;
//                    anchors.leftMargin: 5;

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

                    Loader{
                        id: inputDecoratorLoader1;

                        sourceComponent: Style.textFieldDecorator !==undefined ? Style.textFieldDecorator: emptyDecorator;
                        onLoaded: {
                            if(inputDecoratorLoader1.item){
                                inputDecoratorLoader1.item.rootItem = nameInput;
                            }
                        }
                    }
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

                    Loader{
                        id: inputDecoratorLoader2;

                        sourceComponent: Style.textFieldDecorator !==undefined ? Style.textFieldDecorator: emptyDecorator;
                        onLoaded: {
                            if(inputDecoratorLoader2.item){
                                inputDecoratorLoader2.item.rootItem = descriptionInput;
                            }
                        }
                    }
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

                    Loader{
                        id: inputDecoratorLoader3;

                        sourceComponent: Style.textFieldDecorator !==undefined ? Style.textFieldDecorator: emptyDecorator;
                        onLoaded: {
                            if(inputDecoratorLoader3.item){
                                inputDecoratorLoader3.item.rootItem = pathInput;
                            }
                        }
                    }
                }

                Text {
                    id: titleSettingsPath;

                    anchors.left: parent.left;

                    color: Style.textColor;
                    font.family: Style.fontFamily;
                    font.pixelSize: Style.fontSize_common;

                    text: qsTr("Settings path");

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

                    Loader{
                        id: inputDecoratorLoader4;

                        sourceComponent: Style.textFieldDecorator !==undefined ? Style.textFieldDecorator: emptyDecorator;
                        onLoaded: {
                            if(inputDecoratorLoader4.item){
                                inputDecoratorLoader4.item.rootItem = settingsPathInput;
                            }
                        }
                    }
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

                    Loader{
                        id: inputDecoratorLoader5;

                        sourceComponent: Style.textFieldDecorator !==undefined ? Style.textFieldDecorator: emptyDecorator;
                        onLoaded: {
                            if(inputDecoratorLoader5.item){
                                inputDecoratorLoader5.item.rootItem = argumentsInput;
                            }
                        }
                    }
                }

                Text {
                    id: titleAutoStart;

                    anchors.left: parent.left;

                    color: Style.textColor;
                    font.family: Style.fontFamily;
                    font.pixelSize: Style.fontSize_common;

                    text: qsTr("Autostart (") + (switchAutoStart.checked ? qsTr("true") : qsTr("false")) + ")";

                }

                SwitchCustom {
                    id: switchAutoStart
                    height: Style.itemSizeMedium;
                    width: height * 2
                    backgroundColor: "#D4D4D4"
                    onCheckedChanged: {
                        serviceEditorContainer.doUpdateModel();
                    }
                }
            }//Column bodyColumn
        }//columnContainer
        //
    }
}//Container
