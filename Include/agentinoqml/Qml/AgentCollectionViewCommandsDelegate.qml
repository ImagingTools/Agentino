import QtQuick 2.12
import Acf 1.0
import imtgui 1.0
import imtcolgui 1.0

CollectionViewCommandsDelegateBase {
    id: container;

    removeDialogTitle: qsTr("Deleting an agent");
    removeMessage: qsTr("Delete the selected agent ?");

    onCommandActivated: {
        console.log("AgentCommands onCommandActivated", commandId);
        if (commandId === "Servicies"){
            let itemId = container.collectionViewBase.baseCollectionView.table.getSelectedId();
            let itemName = container.collectionViewBase.baseCollectionView.table.getSelectedName();

            let serviciesStr = qsTr("Servicies of ");
            console.log("Open servicies", itemId)

            // container.documentManager.addDocument({"Id":         itemId,
            //                           "Name":       serviciesStr + itemName,
            //                           "Source":     "../../ServiceCollectionView.qml",
            //                           "CommandsId": "Servicies"});

            container.documentManagerPtr.openDocument(itemId, "Servicies");

                                                      //                                      "Source":     container.collectionViewBase.baseCollectionView.commands.objectViewEditorPath,
//                                      "CommandsId": container.collectionViewBase.baseCollectionView.commands.objectViewEditorCommandsId});

        }
    }

}

