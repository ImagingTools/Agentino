import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0

RemoteCollectionView {
     id: root;

     visibleMetaInfo: false;

     collectionId: "Agents";

     commandsDelegate: AgentCollectionViewCommandsDelegate {
        collectionView: root
     }


     Component.onCompleted: {
        let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
         console.log("documentManagerPtr", documentManagerPtr)
         if (documentManagerPtr){
             root.commandsDelegate.documentManager = documentManagerPtr
             documentManagerPtr.registerDocumentView("Agent", "Agent", singleDocumentWorkspaceView);
             documentManagerPtr.registerDocumentDataController("Agent", agentDataControllerComp);
         }
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
     }

     function onEdit() {
         if (commandsDelegate){
             commandsDelegate.commandHandle("Agent");
         }
     }

     Component {
         id: singleDocumentWorkspaceView

         SingleDocumentWorkspaceView {
             id: singleDocumentManager
            Component.onCompleted: {
                var clientId = root.table.getSelectedIds()[0];
                var clientName = root.table.getSelectedNames()[0];
                MainDocumentManager.registerDocumentManager("Agent", singleDocumentManager);

                addInitialItem(serviceCollectionViewComp, clientName);
            }
         }
     }

     Component {
         id: serviceCollectionViewComp;

         ServiceCollectionView {
             Component.onCompleted: {
                 clientId = root.table.getSelectedIds()[0];
                 clientName = root.table.getSelectedNames()[0];
             }
         }
     }

     Component {
         id: agentDataControllerComp

         DocumentDataController {
             function getDocumentName() {
                return root.table.getSelectedNames()[0];
            }
         }
     }
 }
