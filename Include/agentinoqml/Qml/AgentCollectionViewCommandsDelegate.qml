import QtQuick 2.12
import Acf 1.0
import imtgui 1.0
import imtdocgui 1.0
import imtcolgui 1.0

DocumentCollectionViewDelegate {
    id: container;

    viewTypeId: "Services"
    documentTypeId: "Services"

    removeDialogTitle: qsTr("Deleting an agent");
    removeMessage: qsTr("Delete the selected agent ?");

    onCommandActivated: {
        if (commandId === "Services"){
            console.log("AgentCommands onCommandActivated", commandId);
            let itemId = container.collectionView.table.getSelectedIds()[0];
            let itemName = container.collectionView.table.getSelectedNames()[0];

            let serviciesStr = qsTr("Services of ");
            console.log("Open servicies", itemId)

            let documentManagerPtr = MainDocumentManager.getDocumentManager(container.collectionView.collectionId)
             console.log("documentManagerPtr", documentManagerPtr)
             if (documentManagerPtr){
                 documentManagerPtr.openDocument(itemId, "Services", "Services");
            }
            // container.documentManager.addDocument({"Id":         itemId,
            //                           "Name":       serviciesStr + itemName,
            //                           "Source":     "../../ServiceCollectionView.qml",
            //                           "CommandsId": "Services"});


                                                      //                                      "Source":     container.collectionViewBase.baseCollectionView.commands.objectViewEditorPath,
//                                      "CommandsId": container.collectionViewBase.baseCollectionView.commands.objectViewEditorCommandsId});

        }
    }

    function updateItemSelection(selectedItems){
        console.log("updateItemSelection", selectedItems);
        if (container.collectionView && container.collectionView.commandsController){
            let isEnabled = selectedItems.length > 0;
            let commandsController = container.collectionView.commandsController;
            if(commandsController){
                commandsController.setCommandIsEnabled("Services", isEnabled);
                commandsController.setCommandIsEnabled("Remove", isEnabled);
                commandsController.setCommandIsEnabled("Edit", isEnabled);
            }
        }
    }

}

