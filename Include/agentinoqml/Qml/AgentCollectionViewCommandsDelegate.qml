import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtdocgui 1.0
import imtcolgui 1.0

DocumentCollectionViewDelegate {
    id: container;

    viewTypeId: "AgentView"
    documentTypeId: "Agent"

    removeDialogTitle: qsTr("Deleting an agent");
    removeMessage: qsTr("Delete the selected agent ?");

    function updateItemSelection(selectedItems){
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

    function onEdit(){
        console.log("onEdit")
        let elementsModel = container.collectionView.table.elements;
        if (!elementsModel){
            return;
        }

        let indexes = container.collectionView.table.getSelectedIndexes();
        if (indexes.length > 0){
            let index = indexes[0];
            if (elementsModel.containsKey("id", index)){
                let itemId = elementsModel.getData("id", index);

                let documentManager = MainDocumentManager.getDocumentManager("AgentsSingleDocument");
                if (documentManager){
                    documentManager.openDocument(itemId, "Agent", "AgentEditor");
                }
            }
        }
    }

    function onNew(){
        let documentManager = MainDocumentManager.getDocumentManager("AgentsSingleDocument");
        if (documentManager){
            documentManager.insertNewDocument("Agent", "AgentEditor");
        }
    }
}

