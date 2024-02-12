import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0

RemoteCollectionView {
     id: root;

     property string clientId;
     property string clientName;

     collectionId: "Services";

     onClientIdChanged: {
         commandsRepresentationProvider.commandId = root.collectionId;
         collectionRepresentation.collectionId = root.collectionId;
         let documentManagerPtr = MainDocumentManager.getDocumentManager("Agents")
          if (documentManagerPtr){
              console.log("documentManagerPtr!!!!!!!!!!!!!!!!!!!!!!!!!")
              serviceCommandsDelegate.documentManager = documentManagerPtr
              root.commandsDelegate.documentManager = documentManagerPtr
              documentManagerPtr.registerDocumentView("Service" + root.clientId, "ServiceEditor", serviceEditorComp);
              documentManagerPtr.registerDocumentDataController("Service" + root.clientId, serviceDataControllerComp);
              let newController = serviceDataControllerComp.createObject(root)
              console.log("newController", newController)
          }
         Events.subscribeEvent("ServiceCommandActivated", root.serviceCommandActivated);
     }

     commandsDelegate: ServiceCollectionViewCommandsDelegate {
         id: serviceCommandsDelegate
         collectionView: root
         documentTypeId: "Service" + root.clientId
     }

     function getAdditionalInputParams(){
         let additionInputParams = {}
         additionInputParams["clientId"] = root.clientId;
         return additionInputParams
     }

     commandsController: CommandsRepresentationProvider {
         id: commandsRepresentationProvider
//         commandId: root.collectionId;
         uuid: root.viewId;

         function getAdditionalInputParams(){
             return root.getAdditionalInputParams()
         }
     }

     Component.onCompleted: {

     }

     Component.onDestruction: {
         let documentManagerPtr = MainDocumentManager.getDocumentManager("Agents")
         if (documentManagerPtr){
             documentManagerPtr.unRegisterDocumentView("Service" + root.clientId, "ServiceEditor");
             documentManagerPtr.unRegisterDocumentDataController("Service" + root.clientId);
         }
         Events.unSubscribeEvent("ServiceCommandActivated", root.serviceCommandActivated);
     }

     dataController: CollectionRepresentation {
         id: collectionRepresentation
//         collectionId: root.collectionId;

         additionalFieldIds: root.additionalFieldIds;
         function getAdditionalInputParams(){
             return root.getAdditionalInputParams()
         }
     }

     function serviceCommandActivated(parameters){
         let indexes = root.table.getSelectedIndexes();
         if (indexes.length > 0){
             let index = indexes[0];
             let serviceId = root.table.elements.GetData("Id", index)
             console.log("serviceCommandActivated", parameters, index, serviceId);
             serviceCommandsDelegate.setServiceCommand(parameters, serviceId)
//             servicesController.setServiceCommand(parameters, serviceId)
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
             id: serviceEditor
             commandsController: CommandsRepresentationProvider {
                 commandId: "Service";
                 uuid: serviceEditor.viewId;
                 function getAdditionalInputParams(){
                     console.log("root getAdditionalInputParams");
                     return root.getAdditionalInputParams();
                 }
             }
         }
     }

     Component {
         id: serviceDataControllerComp

         GqlDocumentDataController {
//             documentId: "Service"
//             documentName: "Service name"
             gqlGetCommandId: "ServiceItem"
             gqlUpdateCommandId: "ServiceUpdate"
             gqlAddCommandId: "ServiceAdd"
             function getAdditionalInputParams(){
                 console.log("root getAdditionalInputParams");
                 return root.getAdditionalInputParams();
             }
             function getDocumentName() {
                return root.clientName + " - " + documentName
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
