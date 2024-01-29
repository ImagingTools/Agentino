import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0

CollectionView {
     id: root;

     property string clientId;

     documentName: qsTr("Services");

     function getAdditionalInputParams(){
         let additionInputParams = {}
         additionInputParams["clientId"] = clientId;
         return additionInputParams
     }

     Component.onCompleted: {
         root.commandsDelegatePath = "../ServiceCollectionViewCommandsDelegate.qml";
         Events.subscribeEvent("ServiceCommandActivated", root.serviceCommandActivated);
     }

     onDocumentManagerPtrChanged: {
         if (documentManagerPtr){
             documentManagerPtr.registerDocument("Service", serviceEditorComp);
         }
     }

     onClientIdChanged: {
         console.log("onClientIdChanged", clientId);

         root.commandId = "Services";
     }

     function serviceCommandActivated(parameters){
         let indexes = root.baseCollectionView.table.getSelectedIndexes();
         if (indexes.length > 0){
             let index = indexes[0];
             let serviceId = root.table.elements.GetData("Id", index)
             console.log("serviceCommandActivated", parameters, index, serviceId);
             servicesController.setServiceCommand(parameters, serviceId)
         }
     }

     function selectItem(id, name, index){
         let editorPath = root.getEditorPath();
         let documentTypeId = root.getEditorCommandId();

         if(name === undefined){
             name = " ";
         }

         if (documentTypeId === ""){
             return;
         }

//         name = root.itemName + " - " + name


         let additionInputParams = getAdditionalInputParams()

         console.log("_DEBUG_name", name)

         if (id === ""){
             documentManagerPtr.insertNewDocument(documentTypeId);
         }
         else{
             documentManagerPtr.openDocument(id, documentTypeId, {}, additionInputParams);
         }
     }

     Component {
         id: serviceEditorComp;

         ServiceEditor {
             function getAdditionalInputParams(){
                 console.log("root getAdditionalInputParams");
                 return root.getAdditionalInputParams();
             }
         }
     }

     property GqlModel servicesController: GqlModel {
         function setServiceCommand(commandId, serviceId){
             console.log( "ServiceCollectionView setServiceCommand", commandId, serviceId);
             var query = Gql.GqlRequest("mutation", "Service" + commandId);

             let inputParams = Gql.GqlObject("input");
             inputParams.InsertField("serviceId", serviceId);
             if (Object.keys(root.commandsProvider.additionInputParams).length > 0){
                 let additionParams = Gql.GqlObject("addition");
                 for (let key in root.commandsProvider.additionInputParams){
                     additionParams.InsertField(key, root.commandsProvider.additionInputParams[key]);
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
             this.SetGqlQuery(gqlData);
         }

         onStateChanged: {
             console.log("State:", this.state);
             if (this.state === "Ready"){
                 var dataModelLocal;

                 if (gqlModelBaseContainer.objectViewModel.ContainsKey("errors")){
                     dataModelLocal = gqlModelBaseContainer.objectViewModel.GetData("errors");

                     if (dataModelLocal.ContainsKey(gqlModelBaseContainer.gqlModelObjectView)){
                         dataModelLocal = dataModelLocal.GetData(gqlModelBaseContainer.gqlModelObjectView);
                     }

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

                 dataModelLocal = gqlModelBaseContainer.objectViewModel.GetData("data");

                 if(!dataModelLocal)
                     return;
             }
         }
     }

     SubscriptionClient {
         id: subscriptionClient;

         Component.onCompleted: {
             let subscriptionRequestId = "OnServiceStateChanged"
             var query = Gql.GqlRequest("subscription", subscriptionRequestId);
             var queryFields = Gql.GqlObject("notification");
             queryFields.InsertField("Id");
             query.AddField(queryFields);
             subscriptionManager.registerSubscription(query, subscriptionClient);
         }

         onStateChanged: {
             console.log("SubscriptionClient onStateChanged", state);

             if (state === "Ready"){
                 if (subscriptionClient.ContainsKey("data")){
                     console.log("SubscriptionClient_REady", subscriptionClient.toJSON())
                     root.updateGui();
                 }
             }
         }
     }

}
