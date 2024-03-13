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

    filterMenuVisible: false;

    commandsDelegate: AgentCollectionViewCommandsDelegate {
        collectionView: root
    }

    Component.onCompleted: {
        let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
        if (documentManagerPtr){
            root.commandsDelegate.documentManager = documentManagerPtr

            documentManagerPtr.registerDocumentView("Agent", "AgentView", singleDocumentWorkspaceView);
            documentManagerPtr.registerDocumentDataController("Agent", agentDataControllerComp);

            documentManagerPtr.activeDocumentIndex = -1;
        }
    }

    function onEdit() {
        if (commandsDelegate){
            commandsDelegate.commandHandle("Services");
        }
    }

    Component {
        id: singleDocumentWorkspaceView

        SingleDocumentWorkspaceView {
            id: singleDocumentManager

            anchors.fill: parent;

            Component.onCompleted: {
                MainDocumentManager.registerDocumentManager("Services", singleDocumentManager);

                var clientId = root.table.getSelectedIds()[0];
                var clientName = root.table.getSelectedNames()[0];

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
