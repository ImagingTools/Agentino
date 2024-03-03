import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0

Dialog {
    id: portsDialog;

    width: 500;

    title: qsTr("Edit ports");

    property TreeItemModel portsModel: TreeItemModel {}

    property TreeItemModel tableModel: TreeItemModel {}

    Component.onCompleted: {
        buttonsModel.append({Id: Enums.save, Name:qsTr("Save"), Enabled: true})
        buttonsModel.append({Id: Enums.cancel, Name:qsTr("Cancel"), Enabled: true})

        updateGui();
    }

    onPortsModelChanged: {
        portsDialog.contentItem.tableView.elements = portsDialog.portsModel;
    }

    onFinished: {
        if (buttonId == Enums.save){
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
                    let index = dialogBody.headersModel.InsertNewItem();
                    dialogBody.headersModel.SetData("Id", "Host", index);
                    dialogBody.headersModel.SetData("Name", qsTr("Host"), index);

                    index = dialogBody.headersModel.InsertNewItem();
                    dialogBody.headersModel.SetData("Id", "Port", index);
                    dialogBody.headersModel.SetData("Name", qsTr("Port"), index);

                    index = dialogBody.headersModel.InsertNewItem();
                    dialogBody.headersModel.SetData("Id", "Description", index);
                    dialogBody.headersModel.SetData("Name", qsTr("Description"), index);

                    tableTreeView.headers = dialogBody.headersModel;
                }
            }

            Column {
                id: bodyColumn;

                anchors.verticalCenter: parent.verticalCenter;
                anchors.right: parent.right;
                anchors.left: parent.left;
                anchors.rightMargin: 10;
                anchors.leftMargin: 10;

                width: portsDialog.width;

                TreeItemModel {
                    id: commandsModel;

                    Component.onCompleted: {
                        let index = commandsModel.InsertNewItem();

                        commandsModel.SetData("Id", "Add", index);
                        commandsModel.SetData("Name", "Add", index);
                        commandsModel.SetData("Icon", "Icons/Add", index);
                        commandsModel.SetData("IsEnabled", true, index);
                        commandsModel.SetData("Visible", true, index);

                        index = commandsModel.InsertNewItem();

                        commandsModel.SetData("Id", "Remove", index);
                        commandsModel.SetData("Name", "Remove", index);
                        commandsModel.SetData("Icon", "Icons/Delete", index);
                        commandsModel.SetData("IsEnabled", false, index);
                        commandsModel.SetData("Visible", true, index);

                        commandsDecorator.commandModel = commandsModel;
                    }
                }

                SimpleCommandsDecorator {
                    id: commandsDecorator;

                    width: parent.width;
                    height: 25;

                    onCommandActivated: {
                        if (commandId == "Add"){
                            let index = portsDialog.portsModel.InsertNewItem();

                            portsDialog.portsModel.SetData("Id", UuidGenerator.generateUUID(), index);
                            portsDialog.portsModel.SetData("Name", "", index);
                            portsDialog.portsModel.SetData("Host", "localhost", index);
                            portsDialog.portsModel.SetData("Port", "80", index);
                            portsDialog.portsModel.SetData("Description", "", index);
                        }
                        else if (commandId == "Remove"){
                            let indexes = tableTreeView.getSelectedIndexes();

                            if (indexes.length > 0){
                                let index = indexes[0];

                                portsDialog.portsModel.RemoveItem(index)

                                tableTreeView.resetSelection();
                            }
                        }
                    }
                }

                TextInputCellContentComp {
                    id: textInputComp;
                }

                AuxTable {
                    id: tableTreeView;

                    width: parent.width;
                    height: 200;

                    radius: 0;

                    onHeadersChanged: {
                        tableTreeView.setColumnContentComponent(0, textInputComp)
                        tableTreeView.setColumnContentComponent(1, textInputComp)
                        tableTreeView.setColumnContentComponent(2, textInputComp)
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
