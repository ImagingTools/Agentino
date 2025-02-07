import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import agentinoAgentsSdl 1.0

RemoteCollectionView {
    id: root;

    visibleMetaInfo: false;

    collectionId: "Agents";

    filterMenuVisible: false;

    commandsDelegateComp: Component {AgentCollectionViewCommandsDelegate {
        collectionView: root

		onCommandActivated: {
			if (commandId === "Services"){
				let itemId = root.table.getSelectedIds()[0];

				let indexes = root.table.getSelectedIndexes();
				if (indexes.length > 0){
					let index = indexes[0];
					let id = root.table.elements.getData("Id", index);
					let name = root.table.elements.getData("ComputerName", index);
					let documentManagerPtr = MainDocumentManager.getDocumentManager("Agents")
					if (documentManagerPtr){
						let view = documentManagerPtr.getActiveView();
						view.addFixedView(singleDocumentWorkspaceView, name, id, true);
					}
				}
			}
		}
    }
    }

    Component.onCompleted: {
        let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
		if (documentManagerPtr && root.commandsDelegate){
            root.commandsDelegate.documentManager = documentManagerPtr
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
            documentManager: DocumentManager {}

            Component.onCompleted: {
                MainDocumentManager.registerDocumentManager("Services", documentManager);

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

			onReused: {
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
		gqlCommandId: "OnAgentStatusChanged";

		onMessageReceived: {
			root.doUpdateGui();
		}
    }
}
