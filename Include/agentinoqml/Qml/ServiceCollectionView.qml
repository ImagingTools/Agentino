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
    additionalFieldIds: ["Status"]

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
        uuid: root.viewId;

        function getAdditionalInputParams(){
            return root.getAdditionalInputParams()
        }
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

        additionalFieldIds: root.additionalFieldIds;

        function getAdditionalInputParams(){
            return root.getAdditionalInputParams()
        }
    }

    function serviceCommandActivated(commandId){
        let indexes = root.table.getSelectedIndexes();
        if (indexes.length > 0){
            let index = indexes[0];
            let serviceId = root.table.elements.GetData("Id", index)

            if (root.commandsController){
                commandsController.setCommandIsEnabled("Start", commandId === "Stop");
                commandsController.setCommandIsEnabled("Stop", commandId === "Start");
            }

            serviceCommandsDelegate.setServiceCommand(commandId, serviceId)
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
            gqlGetCommandId: "ServiceItem"
            gqlUpdateCommandId: "ServiceUpdate"
            gqlAddCommandId: "ServiceAdd"

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
                    else{
                        console.log("Stopped" ,status);

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
            if (state === "Ready"){
                console.log("OnServiceStateChanged Ready", subscriptionClient.toJSON());
                if (subscriptionClient.ContainsKey("data")){

                    let dataModel = subscriptionClient.GetData("data")
                    if (dataModel.ContainsKey("OnServiceStateChanged")){
                        dataModel = dataModel.GetData("OnServiceStateChanged")

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

