import QtQuick 2.12
import Acf 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0

DocumentCollectionViewDelegate {
    id: container;

    viewTypeId: "ServiceEditor"
    documentTypeId: "Service"

    removeDialogTitle: qsTr("Deleting an agent");
    removeMessage: qsTr("Delete the selected agent ?");

    function updateItemSelection(selectedItems){
        if (container.collectionView && container.collectionView.commandsController){
            let isEnabled = selectedItems.length > 0;
            let commandsController = container.collectionView.commandsController;
            if(commandsController){
                commandsController.setCommandIsEnabled("Remove", isEnabled);
                commandsController.setCommandIsEnabled("Edit", isEnabled);
                if (isEnabled){
                    let elements = container.collectionView.table.elements;

                    let status = elements.GetData("StatusName", selectedItems[0]);

                    commandsController.setCommandIsEnabled("Start", String(status) !== "Running");
                    commandsController.setCommandIsEnabled("Stop", String(status) === "Running");
                }
            }
        }
    }

    onCommandActivated: {
        if (commandId === "Start" || commandId === "Stop"){
            console.log("ServiceCommands onCommandActivated", commandId);
            Events.sendEvent("ServiceCommandActivated", commandId);
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
        serviceGqlModel.SetGqlQuery(gqlData);
    }

    property GqlModel servicesController: GqlModel {
        id: serviceGqlModel
        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                var dataModelLocal;

                if (this.ContainsKey("errors")){
                    dataModelLocal = this.GetData("errors");

                    let message = ""
                    if (dataModelLocal.ContainsKey("message")){
                        message = dataModelLocal.GetData("message");
                    }

                    let type;
                    if (dataModelLocal.ContainsKey("type")){
                        type = dataModelLocal.GetData("type");
                    }

                    Events.sendEvent("SendError", {"Message": message, "ErrorType": type})

                    return;
                }

                dataModelLocal = this.GetData("data");

                if(!dataModelLocal)
                    return;
            }
        }
    }

}

