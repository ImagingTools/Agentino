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
    }

    function updateModel(){
        serviceEditorContainer.model.SetData("Name", nameInput.text);
        serviceEditorContainer.model.SetData("Description", descriptionInput.text);
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
            }//Column bodyColumn
        }//columnContainer
    }
}//Container
