import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0

ViewBase {

RemoteCollectionView {
     id: root;

     property string clientId;
     property string clientName;

     collectionId: "Services";
     additionalFieldIds: ["Status"]

     onClientIdChanged: {
         if (clientId == ""){
             return
         }
         console.log("onClientIdChanged", clientId)
//         root.collectionId = "Services"
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
                return documentName + "@" + root.clientName
            }
         }
     }

     onHeadersChanged: {
         if (root.table.headers.GetItemsCount() > 0){
              let orderIndex = root.table.getHeaderIndex("StatusName");
             root.table.setColumnContentComponent(orderIndex, stateColumnContentComp);
         }
     }

     Component {
         id: stateColumnContentComp;
         Item {
             id: content
             property var tableCellDelegate
             Image {
                 id: image;

                 anchors.verticalCenter: parent.verticalCenter;
                 anchors.left: parent.left;
                 anchors.leftMargin: 5;

                 width: 9;
                 height: width;

//                 source: "../../../../" + Style.getIconPath("Icons/StateOk2", Icon.State.On, Icon.Mode.Normal);

//                 visible: false;

                 sourceSize.width: width;
                 sourceSize.height: height;
             }

             Text {
                 id: lable;

                 anchors.left: image.right;
                 anchors.leftMargin: Style.size_smallMargin
                 anchors.right: parent.right;
                 anchors.verticalCenter: parent.verticalCenter;

                 font.pixelSize: Style.fontSize_common;
                 font.family: Style.fontFamily;
                 color: Style.textColor;

                 elide: Text.ElideRight;
             }

//             onTableCellDelegateChanged: {
//                 let value = content.tableCellDelegate.getValue();
//                 let rowIndex = tableCellDelegate.rowIndex;
//                 lable.text = value;
//             }

             Component.onCompleted: {
                 let loader = parent;
                 let tableCellDelegate = loader.parent;

                 let rowIndex = tableCellDelegate.rowIndex;

                 if (rowIndex >= 0){
                     let status = root.table.elements.GetData("Status", rowIndex);
                     if (status === "Running"){
                         image.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
                     }
                     else{
                         image.source = "../../../../" + Style.getIconPath("Icons/Stopped", Icon.State.On, Icon.Mode.Normal);
                     }
                 }
                 let value = tableCellDelegate.getValue();
                 if (value !== undefined){
                     lable.text = value;
                 }
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

}
