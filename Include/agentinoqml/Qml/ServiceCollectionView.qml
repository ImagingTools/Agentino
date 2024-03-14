import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0

RemoteCollectionView {
    id: root;

    property string clientId;
    property string clientName;

    filterMenuVisible: false;

    collectionId: "Services";
    additionalFieldIds: ["Description","Status","StatusName"]

    onVisibleChanged: {
        if (visible && table.elements.GetItemsCount() !== 0){
            root.doUpdateGui();
        }
    }

    onClientIdChanged: {
        if (clientId == ""){
            return
        }

        commandsRepresentationProvider.commandId = root.collectionId;
        collectionRepresentation.collectionId = root.collectionId;

        let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
        if (documentManagerPtr){
            serviceCommandsDelegate.documentManager = documentManagerPtr
            root.commandsDelegate.documentManager = documentManagerPtr

            documentManagerPtr.registerDocumentView("Service" + root.clientId, "ServiceEditor", serviceEditorComp);
            documentManagerPtr.registerDocumentDataController("Service" + root.clientId, serviceDataControllerComp);
            documentManagerPtr.registerDocumentValidator("Service" + root.clientId, serviceValidatorComp);
        }
    }

    commandsDelegate: ServiceCollectionViewCommandsDelegate {
        id: serviceCommandsDelegate
        collectionView: root
        documentTypeId: "Service" + root.clientId

        onCommandActivated: {
            if (commandId == "Start" || commandId == "Stop"){
                root.commandsController.setCommandIsEnabled("Start", false);
                root.commandsController.setCommandIsEnabled("Stop", false);
            }
        }
    }

    dataController: CollectionRepresentation {
        id: collectionRepresentation

        additionalFieldIds: root.additionalFieldIds;

        function getAdditionalInputParams(){
            return root.getAdditionalInputParams()
        }
    }

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        uuid: root.viewId;
    }

    Component.onDestruction: {
        let documentManagerPtr = MainDocumentManager.getDocumentManager("Agents")
        if (documentManagerPtr){
            documentManagerPtr.unRegisterDocumentView("Service" + root.clientId, "ServiceEditor");
            documentManagerPtr.unRegisterDocumentDataController("Service" + root.clientId);
        }
    }

    function getAdditionalInputParams(){
        let additionInputParams = {}
        additionInputParams["clientId"] = root.clientId;
        return additionInputParams
    }

    Component {
        id: serviceEditorComp;

        ServiceEditor {
            id: serviceEditor

            commandsDelegate: ViewCommandsDelegateBase {
                view: serviceEditor;
            }

            commandsController: CommandsRepresentationProvider {
                commandId: "Service";
                uuid: serviceEditor.viewId;

                function getAdditionalInputParams(){
                    return root.getAdditionalInputParams();
                }
            }
        }
    }

    Component {
        id: serviceValidatorComp;

        ServiceValidator {
        }
    }

    Component {
        id: serviceDataControllerComp

        GqlDocumentDataController {
            gqlGetCommandId: "ServiceItem"
            gqlUpdateCommandId: "ServiceUpdate"
            gqlAddCommandId: "ServiceAdd"

            subscriptionCommandId: "OnServicesCollectionChanged";

            onHasRemoteChangesChanged: {
                console.log("onHasRemoteChangesChanged", hasRemoteChanges);
                if (hasRemoteChanges){
                    updateDocumentModel();
                }
            }

            function getAdditionalInputParams(){
                return root.getAdditionalInputParams();
            }

            function getDocumentName() {
                let newName = qsTr("<New service>");

                if (documentName !== ""){
                    return documentName + "@" + root.clientName
                }

                return newName + "@" + root.clientName
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

            Component.onCompleted: {
                let loader = parent;
                let tableCellDelegate = loader.parent;

                let rowIndex = tableCellDelegate.rowIndex;
                if (rowIndex >= 0){
                    let status = root.table.elements.GetData("Status", rowIndex);
                    console.log("status" ,status);

                    if (status === "Running"){
                        console.log("Running" ,status);

                        image.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
                    }
                    else if (status === "NotRunning" || status === "Stopping" || status === "Starting"){
                        console.log("Stopped" ,status);

                        image.source = "../../../../" + Style.getIconPath("Icons/Stopped", Icon.State.On, Icon.Mode.Normal);
                    }
                    else{
                        image.source = "../../../../" + Style.getIconPath("Icons/Alert", Icon.State.On, Icon.Mode.Normal);
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
            let subscriptionRequestId = "OnServiceStatusChanged"
            var query = Gql.GqlRequest("subscription", subscriptionRequestId);
            var queryFields = Gql.GqlObject("notification");
            queryFields.InsertField("Id");
            query.AddField(queryFields);

            subscriptionManager.registerSubscription(query, subscriptionClient);
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("OnServiceStatusChanged Ready", subscriptionClient.toJSON());
                if (subscriptionClient.ContainsKey("data")){

                    let dataModel = subscriptionClient.GetData("data")
                    if (dataModel.ContainsKey("OnServiceStatusChanged")){
                        dataModel = dataModel.GetData("OnServiceStatusChanged")

                        let serviceId = dataModel.GetData("serviceId")
                        let serviceStatus = dataModel.GetData("serviceStatus")

                        let elementsModel = root.table.elements;
                    }

                    root.doUpdateGui();
                }
            }
        }
    }
}

