import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0

import agentino 1.0

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
        console.log("TopologyPage onCompleted");

        let documentManager = MainDocumentManager.getDocumentManager("Topology");
        if (documentManager){
            documentManager.registerDocumentView("Service", "ServiceView", serviceEditorComp);
            documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
            documentManager.registerDocumentValidator("Service", serviceValidatorComp);
        }

        itemsTopologyModel.updateModel()
    }

    commandsControllerComp: Component {CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: "Topology"
        uuid: topologyPage.viewId;

        onCommandsModelChanged: {
            setIsToggleable("AutoFit", true);
            setToggled("AutoFit", scheme.autoFit);
        }
    }
    }

    onVisibleChanged: {
        if (topologyPage.visible){
            itemsTopologyModel.updateModel()
        }
    }

    commandsDelegateComp: Component {ServiceCollectionViewCommandsDelegate {
        id: serviceCommandsDelegate
        collectionView: topologyPage
        view: topologyPage;

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
    }

    SchemeView {
        id: scheme

        anchors.left: parent.left;
        anchors.right: metaInfo.left;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        property string selectedService: ""

        onAutoFitChanged: {
            console.log("onAutoFitChanged", scheme.autoFit);
            topologyPage.commandsController.setToggled("AutoFit", scheme.autoFit);
        }

        onModelDataChanged: {
            topologyPage.commandsController.setCommandIsEnabled("Save", true)
        }

        onSelectedIndexChanged: {
            if(objectModel.GetItemsCount() > selectedIndex && selectedIndex >= 0){
                selectedService = objectModel.GetData("Id", selectedIndex);
                let status = objectModel.GetData("Status", selectedIndex);
                topologyPage.commandsController.setCommandIsEnabled("Start", status === "NotRunning")
                topologyPage.commandsController.setCommandIsEnabled("Stop", status === "Running")
                topologyPage.commandsController.setCommandIsEnabled("Edit", true)

                metaInfo.contentVisible = true;
                metaInfoProvider.getMetaInfo(selectedService);
            }
            else{
                selectedService = ""
                topologyPage.commandsController.setCommandIsEnabled("Start", false)
                topologyPage.commandsController.setCommandIsEnabled("Stop", false)
                topologyPage.commandsController.setCommandIsEnabled("Edit", false)

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

        width: 250;
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
            commandsDelegateComp: Component {ViewCommandsDelegateBase {
                view: serviceEditor;
            }
            }

            commandsControllerComp: Component {CommandsRepresentationProvider {
                commandId: "Service";
                uuid: serviceEditor.viewId;
                function getAdditionalInputParams(){
                    return topologyPage.getAdditionalInputParams();
                }
            }}
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
                for (let i = 0; i < scheme.objectModel.GetItemsCount(); i++){
                    let id = scheme.objectModel.GetData("Id", i);
                    if (id == documentId){
                        let name = scheme.objectModel.GetData("MainText", i);

                        return name;
                    }
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
        id: topologySubscriptionClient;

        Component.onCompleted: {
            console.log("topologySubscriptionClient onCompleted", topologySubscriptionClient);

            let subscriptionRequestId = "OnTopologyChanged"
            var query = Gql.GqlRequest("subscription", subscriptionRequestId);
            var queryFields = Gql.GqlObject("notification");
            queryFields.InsertField("Id");
            query.AddField(queryFields);

            Events.sendEvent("RegisterSubscription", {"Query": query, "Client": topologySubscriptionClient});
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("OnTopologyChanged Ready", topologySubscriptionClient.ToJson());

                if (topologySubscriptionClient.ContainsKey("data")){
                    topologyPage.itemsTopologyModel.updateModel()
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

            Events.sendEvent("RegisterSubscription", {"Query": query, "Client": subscriptionClient});
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("TopologyPage OnServiceStatusChanged Ready", subscriptionClient.ToJson());
                if (subscriptionClient.ContainsKey("data")){
                    let dataModel = subscriptionClient.GetData("data")
                    if (dataModel.ContainsKey("OnServiceStatusChanged")){
                        dataModel = dataModel.GetData("OnServiceStatusChanged")
                        let serviceId = dataModel.GetData("serviceId")
                        let serviceStatus = dataModel.GetData("serviceStatus")
                        let dependencyStatus
                        console.log("serviceStatus", serviceStatus)

                        let index = scheme.findModelIndex(serviceId);
                        scheme.objectModel.SetData("Status", serviceStatus, index);
                        if (serviceStatus === ServiceStatus.s_Running){
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Running", index);
                        }
                        else if (serviceStatus === ServiceStatus.s_NotRunning || serviceStatus === ServiceStatus.s_Stopping || serviceStatus === ServiceStatus.s_Starting){
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Stopped", index);
                        }
                        else{
                            scheme.objectModel.SetData("IconUrl_1", "Icons/Alert", index);
                        }
                        if (index === scheme.selectedIndex){
                            topologyPage.commandsController.setCommandIsEnabled("Start", serviceStatus === ServiceStatus.s_NotRunning);
                            topologyPage.commandsController.setCommandIsEnabled("Stop", serviceStatus === ServiceStatus.s_Running);
                        }
                        let dependencyStatusModel = dataModel.GetData(DependencyStatus.s_Key)
                        for (let i = 0; i < dependencyStatusModel.GetItemsCount(); i++){
                            serviceId = dependencyStatusModel.GetData("id", i);
                            dependencyStatus = dependencyStatusModel.GetData(DependencyStatus.s_Key, i)
                            index = scheme.findModelIndex(serviceId);

                            if (dependencyStatus === DependencyStatus.s_NotAllRunning){
                                scheme.objectModel.SetData("IconUrl_2", "Icons/Error", index);
                            }
                            else if (dependencyStatus === DependencyStatus.s_Undefined) {
                                scheme.objectModel.SetData("IconUrl_2", "Icons/Warning", index);
                            }
                            else {
                                scheme.objectModel.SetData("IconUrl_2", "", index);
                            }

                            if (serviceId === scheme.selectedService){
                                metaInfoProvider.getMetaInfo(scheme.selectedService);
                            }
                        }

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
            inputParams.InsertField ("Item", scheme.objectModel.ToJson());

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
                            topologyPage.commandsController.setCommandIsEnabled("Save", false)
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

                console.log("GetTopology ready:", topolodyModel.ToJson())

                if (topolodyModel.ContainsKey("data")){
                    dataModelLocal = topolodyModel.GetData("data");
                    if (dataModelLocal.ContainsKey("GetTopology")){
                        dataModelLocal = dataModelLocal.GetData("GetTopology");
                        if (dataModelLocal.ContainsKey("items")){
                            scheme.objectModel = dataModelLocal.GetData("items");
                            scheme.requestPaint()
                            topologyPage.commandsController.setCommandIsEnabled("Save", false)
                        }
                    }
                }
            }
        }
    }
}
