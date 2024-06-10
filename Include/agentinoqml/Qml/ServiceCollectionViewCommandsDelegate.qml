import QtQuick 2.12
import Acf 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import agentino 1.0

DocumentCollectionViewDelegate {
    id: container;

    viewTypeId: "ServiceEditor"
    documentTypeId: "Service"

    removeDialogTitle: qsTr("Deleting an service");
    removeMessage: qsTr("Delete the selected service ?");

    function updateItemSelection(selectedItems){
        console.log("updateItemSelection", selectedItems);

        if (container.collectionView && container.collectionView.commandsController){
            let isEnabled = selectedItems.length > 0;
            let commandsController = container.collectionView.commandsController;
            if(commandsController){
                commandsController.setCommandIsEnabled("Remove", isEnabled);
                commandsController.setCommandIsEnabled("Edit", isEnabled);
                if (isEnabled){
                    let elements = container.collectionView.table.elements;

                    let status = elements.getData("Status", selectedItems[0]);

                    commandsController.setCommandIsEnabled("Start", String(status) === ServiceStatus.s_NotRunning);
                    commandsController.setCommandIsEnabled("Stop", String(status) === ServiceStatus.s_Running);
                }
            }
        }
    }

    onCommandActivated: {
        if (commandId == "Start" || commandId == "Stop"){
            let commandsController = container.collectionView.commandsController;

            commandsController.setCommandIsEnabled("Start", false);
            commandsController.setCommandIsEnabled("Stop", false);
        }

        if (commandId === "Start"){
            onStart();
        }
        else if (commandId === "Stop"){
            onStop();
        }
    }

    function onStart(){
        let elements = container.collectionView.table.elements;
        let indexes = container.collectionView.table.getSelectedIndexes();
        if (indexes.length > 0){
            let index = indexes[0];
            let serviceId = elements.getData("Id", index)

            setServiceCommand("Start", serviceId);
        }
    }

    function onStop(){
        let elements = container.collectionView.table.elements;
        let indexes = container.collectionView.table.getSelectedIndexes();
        if (indexes.length > 0){
            let index = indexes[0];
            let serviceId = elements.getData("Id", index)

            setServiceCommand("Stop", serviceId);
        }
    }

    function setServiceCommand(commandId, serviceId){
        console.log( "ServiceCollectionView setServiceCommand", commandId, serviceId);
        var query = Gql.GqlRequest("mutation", "Service" + commandId);

        let inputParams = Gql.GqlObject("input");
        inputParams.InsertField("serviceId", serviceId);
        let additionInputParams = container.collectionView.getAdditionalInputParams()
        if (Object.keys(additionInputParams).length > 0){
            let additionParams = Gql.GqlObject("addition");
            for (let key in additionInputParams){
                additionParams.InsertField(key, additionInputParams[key]);
            }
            inputParams.InsertFieldObject(additionParams);
        }
        query.AddParam(inputParams);

        var queryField = Gql.GqlObject("serviceStatus");
        queryField.InsertField("serviceId");
        queryField.InsertField("status");
        query.AddField(queryField);

        var gqlData = query.GetQuery();
        console.log("ServiceCollectionView setServiceCommand query ", gqlData);
        serviceGqlModel.setGqlQuery(gqlData);
    }

    function setupContextMenu(){
        let commandsController = collectionView.commandsController;
        if (commandsController){
            container.contextMenuModel.clear();

            let canEdit = commandsController.commandExists("Edit");
            let canRemove = commandsController.commandExists("Remove");

            if (canEdit){
                let index = container.contextMenuModel.insertNewItem();

                container.contextMenuModel.setData("Id", "Edit", index);
                container.contextMenuModel.setData("Name", qsTr("Edit"), index);
                container.contextMenuModel.setData("Icon", "Icons/Edit", index);
            }

            if (canRemove){
                let index = container.contextMenuModel.insertNewItem();

                container.contextMenuModel.setData("Id", "Remove", index);
                container.contextMenuModel.setData("Name", qsTr("Remove"), index);
                container.contextMenuModel.setData("Icon", "Icons/Delete", index);
            }

            container.contextMenuModel.refresh();
        }
    }

    property GqlModel servicesController: GqlModel {
        id: serviceGqlModel
        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){

                console.log("servicesController: ", this.toJson());
                var dataModelLocal;

                if (this.containsKey("errors")){
                    dataModelLocal = this.getData("errors");

                    if (dataModelLocal.containsKey("ServiceStart")){
                        dataModelLocal = dataModelLocal.getData("ServiceStart");
                    }
                    else if (dataModelLocal.containsKey("ServiceStop")){
                        dataModelLocal = dataModelLocal.getData("ServiceStop");
                    }

                    let message = ""
                    if (dataModelLocal.containsKey("message")){
                        message = dataModelLocal.getData("message");
                    }

                    ModalDialogManager.showWarningDialog(message)

                    return;
                }

                dataModelLocal = this.getData("data");

                if(!dataModelLocal)
                    return;
            }
        }
    }

}

