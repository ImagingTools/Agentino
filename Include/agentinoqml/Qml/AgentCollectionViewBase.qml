import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import agentinogqlSdl 1.0

RemoteCollectionView {
    id: root;

    visibleMetaInfo: false;

    collectionId: "Agents";

    filterMenuVisible: false;

    commandsDelegateComp: Component {AgentCollectionViewCommandsDelegate {
        collectionView: root
    }
    }

    Component.onCompleted: {
        let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
        if (documentManagerPtr){
            root.commandsDelegate.documentManager = documentManagerPtr

            documentManagerPtr.registerDocumentView("Agent", "AgentView", singleDocumentWorkspaceView);
            documentManagerPtr.registerDocumentDataController("Agent", agentDataControllerComp);
        }
    }

    onHeadersChanged: {
        if (root.table.headers.getItemsCount() > 0){
            let orderIndex = root.table.getHeaderIndex("Status");
            root.table.setColumnContentComponent(orderIndex, stateColumnContentComp);
        }
    }

    function onEdit() {
        if (root.commandsDelegate){
            root.commandsDelegate.commandHandle("Services");
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

    Component {
        id: stateColumnContentComp;
        TableCellDelegateBase {
            id: content

            Image {
                id: image;

                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: parent.left;
                anchors.leftMargin: 5;

                width: 9;
                height: width;

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

            onRowIndexChanged: {
                if (rowIndex >= 0){
                    let status = root.table.elements.getData("Status", rowIndex);

                    if (status === "Connected"){
                        image.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
                    }
                    else if (status === "Disconnected"){
                        image.source = "../../../../" + Style.getIconPath("Icons/Stopped", Icon.State.On, Icon.Mode.Normal);
                    }
                    else{
                        image.source = "../../../../" + Style.getIconPath("Icons/Alert", Icon.State.On, Icon.Mode.Normal);
                    }
                }

                let value = getValue();
                if (value !== undefined){
                    lable.text = value;
                }
            }
        }
    }

    SubscriptionClient {
        id: subscriptionClient;

        Component.onCompleted: {
            let subscriptionRequestId = "OnAgentStatusChanged"
            var query = Gql.GqlRequest("subscription", subscriptionRequestId);
            var queryFields = Gql.GqlObject("notification");
            queryFields.InsertField("Id");
            query.AddField(queryFields);

            Events.sendEvent("RegisterSubscription", {"Query": query, "Client": subscriptionClient});
        }

        onStateChanged: {
            console.log("OnAgentStatusChanged", state);

            if (state === "Ready"){
                if (subscriptionClient.containsKey("data")){
                    root.doUpdateGui();
                }
            }
        }
    }
}
