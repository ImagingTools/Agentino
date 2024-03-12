import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0


ViewBase {
    id: topologyPage;

    anchors.fill: parent;
    clip: true;

    property TreeItemModel objectModel: TreeItemModel{};

    //for scrollBars
    property real originX: 0;
    property real originY: 0;
    //for scrollBars

    Component.onCompleted: {
        let documentManager = MainDocumentManager.getDocumentManager("Topology");
        if (documentManager){
            documentManager.registerDocumentView("Service", "ServiceView", serviceEditorComp);
            documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
            documentManager.registerDocumentValidator("Service", serviceValidatorComp);
        }
    }

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: "Topology"
        uuid: topologyPage.viewId;

        onCommandsModelChanged: {
            setIsToggleable("AutoFit", true);
            setToggled("AutoFit", scheme.autoFit);
        }
    }

    onVisibleChanged: {
        if (topologyPage.visible){
            itemsTopologyModel.updateModel()
        }
    }

    commandsDelegate: ServiceCollectionViewCommandsDelegate {
        id: serviceCommandsDelegate
        collectionView: topologyPage

        onCommandActivated: {
            if (commandId === "Save"){
                saveModel.save()
            }
            else if (commandId === "ZoomIn"){
                scheme.setAutoFit(false);

                scheme.zoomIn();
            }
            else if (commandId === "ZoomOut"){
                scheme.setAutoFit(false);

                scheme.zoomOut();
            }
            else if (commandId === "ZoomReset"){
                scheme.resetZoom();
            }
            else if (commandId === "AutoFit"){
                scheme.setAutoFit(!scheme.autoFit);
            }
        }

        function onStart(){
            if(scheme.objectModel.GetItemsCount() > scheme.selectedIndex && scheme.selectedIndex >= 0){
                let serviceId = scheme.objectModel.GetData("Id", scheme.selectedIndex);

                serviceCommandsDelegate.setServiceCommand("Start", serviceId);
            }
        }

        function onStop(){
            if(scheme.objectModel.GetItemsCount() > scheme.selectedIndex && scheme.selectedIndex >= 0){
                let serviceId = scheme.objectModel.GetData("Id", scheme.selectedIndex);

                serviceCommandsDelegate.setServiceCommand("Stop", serviceId);
            }
        }

        function onEdit(){
            scheme.goInside()
        }
    }

    SchemeView {
        id: scheme

        anchors.left: parent.left;
        anchors.right: metaInfo.left;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        property string selectedService: ""

        onAutoFitChanged: {
            commandsRepresentationProvider.setToggled("AutoFit", scheme.autoFit);
        }

        onModelDataChanged: {
            commandsRepresentationProvider.setCommandIsEnabled("Save", true)
        }

        onSelectedIndexChanged: {
            if(objectModel.GetItemsCount() > selectedIndex && selectedIndex >= 0){
                selectedService = objectModel.GetData("Id", selectedIndex);
                let status = objectModel.GetData("Status", selectedIndex);
                commandsRepresentationProvider.setCommandIsEnabled("Start", status !== "Running")
                commandsRepresentationProvider.setCommandIsEnabled("Stop", status === "Running")
                commandsRepresentationProvider.setCommandIsEnabled("Edit", true)

                metaInfo.contentVisible = true;
                metaInfoProvider.getMetaInfo(selectedService);
            }
            else{
                selectedService = ""
                commandsRepresentationProvider.setCommandIsEnabled("Start", false)
                commandsRepresentationProvider.setCommandIsEnabled("Stop", false)
                commandsRepresentationProvider.setCommandIsEnabled("Edit", false)

                metaInfo.contentVisible = false;
            }
        }

        function goInside(){
            let documentManager = MainDocumentManager.getDocumentManager("Topology");
            if (documentManager){
                if(objectModel.GetItemsCount() > selectedIndex && selectedIndex >= 0){
                    let serviceId = objectModel.GetData("Id", selectedIndex);
                    documentManager.openDocument(serviceId, "Service", "ServiceView");
                }
            }
        }
    }

    MetaInfo {
        id: metaInfo;

        anchors.right: parent.right;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        width: 200;
    }

    MetaInfoProvider {
        id: metaInfoProvider;

        getMetaInfoGqlCommand: "GetServiceMetaInfo";

        onMetaInfoModelChanged: {
            metaInfo.metaInfoModel = metaInfoModel;
        }

        function getAdditionalInputParams(){
            return topologyPage.getAdditionalInputParams();
        }
    }

    function getAdditionalInputParams(){
        let additionInputParams = {}

        if(scheme.selectedIndex >= 0){
            let agentId = scheme.objectModel.GetData("AgentId", scheme.selectedIndex);
            additionInputParams["clientId"] = agentId;
        }

        return additionInputParams
    }

    Component {
        id: serviceValidatorComp;

        ServiceValidator {
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
                    return topologyPage.getAdditionalInputParams();
                }
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+S";

        Component.onCompleted: {
            console.log("shortcut onCompleted", enabled);
        }

        Component.onDestruction: {
            console.log("shortcut onDestruction", enabled);
        }

        onEnabledChanged: {
            console.log("shortcut onEnabledChanged", enabled);
        }

        onActivated: {
            console.log("Ctrl+S onActivated");
            if (!singleDocumentData.isDirty){
                return
            }

            singleDocumentData.commandHandle("Save");
        }
    }

    Component {
        id: serviceDataControllerComp

        GqlDocumentDataController {
            gqlGetCommandId: "ServiceItem"
            gqlUpdateCommandId: "ServiceUpdate"
            gqlAddCommandId: "ServiceAdd"

            subscriptionCommandId: "OnServicesCollectionChanged";

            function getAdditionalInputParams(){
                return topologyPage.getAdditionalInputParams();
            }

            function getDocumentName(){
                if (scheme.selectedIndex >= 0){
                    let name = scheme.objectModel.GetData("MainText", scheme.selectedIndex);

                    return name;
                }

                return "";
            }

            onHasRemoteChangesChanged: {
                console.log("Topology onHasRemoteChangesChanged", hasRemoteChanges);
                if (hasRemoteChanges){
                    updateDocumentModel();
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
                console.log("TopologyPage OnServiceStatusChanged Ready", subscriptionClient.toJSON());
                if (subscriptionClient.ContainsKey("data")){
                    let dataModel = subscriptionClient.GetData("data")
                    if (dataModel.ContainsKey("OnServiceStatusChanged")){
                        dataModel = dataModel.GetData("OnServiceStatusChanged")
                        let serviceId = dataModel.GetData("serviceId")
                        let serviceStatus = dataModel.GetData("serviceStatus")
                        console.log("serviceStatus", serviceStatus)

                        let index = scheme.findModelIndex(serviceId);
                        scheme.objectModel.SetData("Status", serviceStatus, index);
                        if (serviceStatus == "Running"){
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Running", index);
                        }
                        else if (serviceStatus === "NotRunning" || serviceStatus === "Stopping" || serviceStatus === "Starting"){
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Stopped", index);
                        }
                        else{
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Alert", index);
                        }

                        topologyPage.commandsController.setCommandIsEnabled("Start", serviceStatus !== "Running");
                        topologyPage.commandsController.setCommandIsEnabled("Stop", serviceStatus === "Running");

                        scheme.requestPaint()
                    }
                }
            }
        }
    }

    property GqlModel topologySaveModel: GqlModel {
        id: saveModel

        function save(){
            var query = Gql.GqlRequest("mutation", "SaveTopology");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField ("Item", scheme.objectModel.toJSON());

            query.AddParam(inputParams);

            var queryFields = Gql.GqlObject("updatedNotification");
            queryFields.InsertField("Id");
            query.AddField(queryFields);

            var gqlData = query.GetQuery();
            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                if (saveModel.ContainsKey("data")){
                    let dataModelLocal = saveModel.GetData("data");
                    if (dataModelLocal.ContainsKey("SaveTopology")){
                        dataModelLocal = dataModelLocal.GetData("SaveTopology");
                        if (dataModelLocal.ContainsKey("notification")){
                            commandsRepresentationProvider.setCommandIsEnabled("Save", false)
                        }
                    }
                }

            }
        }
    }


    property GqlModel itemsTopologyModel: GqlModel {
        id: topolodyModel

        function updateModel() {
            console.log( "topologyPage updateModel", "GetTopology");
            var query = Gql.GqlRequest("query", "GetTopology");
            var queryFields = Gql.GqlObject("items");

            query.AddField(queryFields);

            var gqlData = query.GetQuery();

            console.log("topologyPage query ", gqlData);
            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                var dataModelLocal;
                if (topolodyModel.ContainsKey("errors")){
                    return;
                }

                console.log("GetTopology ready:", topolodyModel.toJSON())

                if (topolodyModel.ContainsKey("data")){
                    dataModelLocal = topolodyModel.GetData("data");
                    if (dataModelLocal.ContainsKey("GetTopology")){
                        dataModelLocal = dataModelLocal.GetData("GetTopology");
                        if (dataModelLocal.ContainsKey("items")){
                            scheme.objectModel = dataModelLocal.GetData("items");
                            scheme.requestPaint()
                            commandsRepresentationProvider.setCommandIsEnabled("Save", false)
                        }
                    }
                }
            }
            else if (this.state === "Error"){
            }
        }
    }

}
