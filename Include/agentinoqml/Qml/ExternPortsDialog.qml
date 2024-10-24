import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0

Dialog {
    id: portsDialog;

    width: 500;

    title: qsTr("Edit ports");

    property TreeItemModel portsModel: TreeItemModel {}

    Component.onCompleted: {
        buttonsModel.append({Id: Enums.save, Name:qsTr("Save"), Enabled: true})
        buttonsModel.append({Id: Enums.cancel, Name:qsTr("Cancel"), Enabled: true})

        updateGui();
    }

    onPortsModelChanged: {
        portsDialog.contentItem.tableView.elements = portsDialog.portsModel;
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
                    dialogBody.headersModel.setData("Id", "Scheme", index);
                    dialogBody.headersModel.setData("Name", qsTr("Scheme"), index);

                    index = dialogBody.headersModel.insertNewItem();
                    dialogBody.headersModel.setData("Id", "Host", index);
                    dialogBody.headersModel.setData("Name", qsTr("Host"), index);

                    index = dialogBody.headersModel.insertNewItem();
                    dialogBody.headersModel.setData("Id", "Port", index);
                    dialogBody.headersModel.setData("Name", qsTr("Port"), index);

                    index = dialogBody.headersModel.insertNewItem();
                    dialogBody.headersModel.setData("Id", "Description", index);
                    dialogBody.headersModel.setData("Name", qsTr("Description"), index);

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
                            let index = portsDialog.portsModel.insertNewItem();

                            portsDialog.portsModel.setData("Id", UuidGenerator.generateUUID(), index);
                            portsDialog.portsModel.setData("Name", "", index);
                            portsDialog.portsModel.setData("Scheme", "http", index);
                            portsDialog.portsModel.setData("Host", "localhost", index);
                            portsDialog.portsModel.setData("Port", "80", index);
                            portsDialog.portsModel.setData("Description", "", index);
                        }
                        else if (commandId == "Remove"){
                            let indexes = tableTreeView.getSelectedIndexes();
                            if (indexes.length > 0){
                                let index = indexes[0];

                                portsDialog.portsModel.removeItem(index)

                                tableTreeView.resetSelection();
                            }
                        }
                    }
                }

                TextInputCellContentComp {
                    id: textInputComp;
                }

                Component {
                    id: comboBoxCellContentComp;
                    ComboBoxCellContentComp {
                        model: schemesModel;
                    }
                }

                TreeItemModel {
                    id: schemesModel;

                    Component.onCompleted: {
                        let index = schemesModel.insertNewItem();
                        schemesModel.setData("Scheme", "http", index);

                        index = schemesModel.insertNewItem();
                        schemesModel.setData("Scheme", "https", index);

                        index = schemesModel.insertNewItem();
                        schemesModel.setData("Scheme", "ws", index);

                        index = schemesModel.insertNewItem();
                        schemesModel.setData("Scheme", "wss", index);
                    }
                }

                Table {
                    id: tableTreeView;

                    width: parent.width;
                    height: 200;

                    radius: 0;

                    onHeadersChanged: {
                        tableTreeView.setColumnContentComponent(0, comboBoxCellContentComp)
                        tableTreeView.setColumnContentComponent(1, textInputComp)
                        tableTreeView.setColumnContentComponent(2, textInputComp)
                        tableTreeView.setColumnContentComponent(3, textInputComp)
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
