import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtcolgui 1.0

 CollectionView {
     id: root;

     visibleMetaInfo: false;

     documentName: qsTr("Agents");

     Component.onCompleted: {
         root.commandsDelegatePath = "../AgentCollectionViewCommandsDelegate.qml";

         root.commandId = "Agents";
     }

     onDocumentManagerPtrChanged: {
         if (documentManagerPtr){
             documentManagerPtr.registerDocument("Servicies", serviceCollectionViewComp);
         }
     }

     function fillContextMenuModel(){
         contextMenuModel.clear();

         contextMenuModel.append({"Id": "Servicies", "Name": qsTr("Servicies"), "IconSource": "../../../../" + Style.getIconPath("Icons/Workflow", "On", "Normal")});
         contextMenuModel.append({"Id": "Edit", "Name": qsTr("Edit"), "IconSource": "../../../../" + Style.getIconPath("Icons/Edit", "On", "Normal")});
         contextMenuModel.append({"Id": "Remove", "Name": qsTr("Remove"), "IconSource": "../../../../" + Style.getIconPath("Icons/Remove", "On", "Normal")});
     }

     function selectItem(id, name){
         console.log("CollectionView selectItem", id, name);

         let editorPath = root.getEditorPath();
         let documentTypeId = root.getEditorCommandId();

         if (documentTypeId === ""){
             console.error("Unable to select item documentTypeId is invalid");
             return;
         }

         if (!name){
             name = "";
         }

         documentManagerPtr.insertNewDocument(documentTypeId, {"clientId": id});
         // documentManagerPtr.openDocument(id, documentTypeId, {"clientId": id});
     }

     Component {
         id: serviceCollectionViewComp;

         ServiceCollectionView {
         }
     }
 }
