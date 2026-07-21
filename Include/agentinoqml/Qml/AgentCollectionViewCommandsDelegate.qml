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

    // enrollmentViewModel is set by AgentCollectionViewBase.qml; while a decision mutation is
    // in flight the action commands are disabled so a slow response can't be double-fired.
    property var enrollmentViewModel: null

    function isConnectionStatus(status){
        return status === "Connected"
                    || status === "Disconnected"
                    || status === "Undefined";
    }

    // The "status" column already carries the enrollment status verbatim (Pending/Suspended/
    // Rejected/Revoked) whenever the agent isn't Approved, and a live connection literal
    // (Connected/Disconnected/Undefined) once it is - see
    // CAgentCollectionControllerComp::CreateRepresentationFromObject. So the enrollment state
    // of a row is just: a connection literal means "Approved", anything else is itself.
    function selectedAgentStatus(){
        if (!container.collectionView || !container.collectionView.table){
            return "";
        }
        let elementsModel = container.collectionView.table.elements;
        if (!elementsModel){
            return "";
        }
        let indexes = container.collectionView.table.getSelectedIndexes();
        if (indexes.length === 0){
            return "";
        }
        let index = indexes[0];
        if (!elementsModel.containsKey("status", index)){
            return "Approved";
        }
        let status = elementsModel.getData("status", index);
        return container.isConnectionStatus(status) ? "Approved" : status;
    }

    function isSelectedAgentApproved(){
        return container.collectionView
                    && container.collectionView.table
                    && container.collectionView.table.getSelectedIndexes().length > 0
                    && container.selectedAgentStatus() === "Approved";
    }

    function updateItemSelection(selectedItems){
        if (container.collectionView && container.collectionView.commandsController){
            let isEnabled = selectedItems.length > 0;
            let isApproved = isEnabled && container.isSelectedAgentApproved();
            let commandsController = container.collectionView.commandsController;
            if(commandsController){
                commandsController.setCommandIsEnabled("Remove", isEnabled);
                commandsController.setCommandIsEnabled("Edit", isApproved);
                commandsController.setCommandIsEnabled("Terminal", isApproved);

                let decisionBusy = container.enrollmentViewModel && container.enrollmentViewModel.loading;
                let status = isEnabled && !decisionBusy ? container.selectedAgentStatus() : "";
                commandsController.setCommandIsEnabled("Approve", status === "Pending");
                commandsController.setCommandIsEnabled("Reject", status === "Pending");
                commandsController.setCommandIsEnabled("Suspend", status === "Approved");
                commandsController.setCommandIsEnabled("Resume", status === "Suspended");
                commandsController.setCommandIsEnabled("Revoke", status === "Approved" || status === "Suspended");
                commandsController.setCommandIsEnabled("Reset", status === "Rejected" || status === "Revoked");
            }
        }
    }

    function onEdit(){
        if (!container.isSelectedAgentApproved()){
            return;
        }
        let elementsModel = container.collectionView.table.elements;
        if (!elementsModel){
            return;
        }

        let indexes = container.collectionView.table.getSelectedIndexes();
        if (indexes.length > 0){
            let index = indexes[0];
            if (elementsModel.containsKey("id", index)){
                let itemId = elementsModel.getData("id", index);
                // Initial tab title (MultiDocWorkspaceView.onDocumentAdded uses this verbatim) -
                // an empty name here is why new agent tabs opened as "<no name>".
                let itemName = elementsModel.containsKey("name", index) ? elementsModel.getData("name", index) : "";

                let documentManager = MainDocumentService.getDocumentService("Agents");
                if (documentManager){
                    documentManager.openDocument(itemId, "Agent", itemName);
                }
            }
        }
    }

    function onNew(){
        let documentManager = MainDocumentService.getDocumentService("Agents");
        if (documentManager){
            documentManager.insertNewDocument("Agent");
        }
    }

    Connections {
        target: container.enrollmentViewModel
        function onLoadingChanged(){
            if (container.collectionView && container.collectionView.table){
                container.updateItemSelection(container.collectionView.table.getSelectedIndexes());
            }
        }
    }
}
