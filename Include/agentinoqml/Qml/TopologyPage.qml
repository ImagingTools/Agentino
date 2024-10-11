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
    property var documentManager: MainDocumentManager.getDocumentManager("Topology");

    //for scrollBars
    property real originX: 0;
    property real originY: 0;
    //for scrollBars

    Component.onCompleted: {
        if (documentManager){
            documentManager.registerDocumentView("Service", "ServiceView", serviceEditorComp);
            documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
            documentManager.registerDocumentValidator("Service", serviceValidatorComp);
        }

        itemsTopologyModel.updateModel()
    }

    commandsControllerComp: Component {CommandsPanelController {
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
            if(scheme.objectModel.getItemsCount() > scheme.selectedIndex && scheme.selectedIndex >= 0){
                let serviceId = scheme.objectModel.getData("id", scheme.selectedIndex);

                serviceCommandsDelegate.setServiceCommand("Start", serviceId);
            }
        }

        function onStop(){
            if(scheme.objectModel.getItemsCount() > scheme.selectedIndex && scheme.selectedIndex >= 0){
                let serviceId = scheme.objectModel.getData("id", scheme.selectedIndex);

                serviceCommandsDelegate.setServiceCommand("Stop", serviceId);
            }
        }

        function onEdit(){
            scheme.goInside()
        }

        function getHeaders(){
            return topologyPage.getHeaders();
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
            topologyPage.commandsController.setToggled("AutoFit", scheme.autoFit);
        }

        onModelDataChanged: {
            topologyPage.commandsController.setCommandIsEnabled("Save", true)
        }

        onSelectedIndexChanged: {
            if(objectModel.getItemsCount() > selectedIndex && selectedIndex >= 0){
                selectedService = objectModel.getData("id", selectedIndex);
                let status = objectModel.getData(ServiceStatus.s_Key, selectedIndex);
                topologyPage.commandsController.setCommandIsEnabled("Start", status === ServiceStatus.s_NotRunning)
                topologyPage.commandsController.setCommandIsEnabled("Stop", status === ServiceStatus.s_Running)
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
                if(objectModel.getItemsCount() > selectedIndex && selectedIndex >= 0){
                    let serviceId = objectModel.getData("id", selectedIndex);
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

        function getHeaders(){
            return topologyPage.getHeaders();
        }
    }

    function getHeaders(){
        let additionInputParams = {}

        if(scheme.selectedIndex >= 0){
            let agentId = scheme.objectModel.getData("agentId", scheme.selectedIndex);
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
            documentManager: topologyPage.documentManager;
            commandsDelegateComp: Component {ViewCommandsDelegateBase {
                view: serviceEditor;
            }
            }

            commandsControllerComp: Component {CommandsPanelController {
                commandId: "Service";
                uuid: serviceEditor.viewId;
                function getHeaders(){
                    return topologyPage.getHeaders();
                }
            }}

            Component.onCompleted: {
                let agentId = scheme.objectModel.getData("agentId", scheme.selectedIndex);
                serviceEditor.agentId = agentId;
            }

            function getHeaders(){
                return topologyPage.getHeaders();
            }
        }
    }

    Component {
        id: serviceDataControllerComp

        GqlDocumentDataController {
            gqlGetCommandId: "ServiceItem"
            gqlUpdateCommandId: "ServiceUpdate"
            gqlAddCommandId: "ServiceAdd"

            subscriptionCommandId: "OnServicesCollectionChanged";

            function getHeaders(){
                return topologyPage.getHeaders();
            }

            function getDocumentName(){
                for (let i = 0; i < scheme.objectModel.getItemsCount(); i++){
                    let id = scheme.objectModel.getData("id", i);
                    if (id == documentId){
                        let name = scheme.objectModel.getData("mainText", i);

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
            queryFields.InsertField("id");
            query.AddField(queryFields);

            Events.sendEvent("RegisterSubscription", {"Query": query, "Client": topologySubscriptionClient});
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("OnTopologyChanged Ready", topologySubscriptionClient.toJson());

                if (topologySubscriptionClient.containsKey("data")){
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
            queryFields.InsertField("id");
            query.AddField(queryFields);

            Events.sendEvent("RegisterSubscription", {"Query": query, "Client": subscriptionClient});
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("TopologyPage OnServiceStatusChanged Ready", subscriptionClient.toJson());
                if (subscriptionClient.containsKey("data")){
                    let dataModel = subscriptionClient.getData("data")
                    if (dataModel.containsKey("OnServiceStatusChanged")){
                        dataModel = dataModel.getData("OnServiceStatusChanged")
                        let serviceId = dataModel.getData("serviceId")
                        let serviceStatus = dataModel.getData(ServiceStatus.s_Key)
                        let dependencyStatus
                        console.log(ServiceStatus.s_Key, serviceStatus)

                        let index = scheme.findModelIndex(serviceId);
                        scheme.objectModel.setData(ServiceStatus.s_Key, serviceStatus, index);
                        if (serviceStatus === ServiceStatus.s_Running){
                            scheme.objectModel.setData(TopologyModel.s_IconUrl_1, "Icons/Running", index);
                        }
                        else if (serviceStatus === ServiceStatus.s_NotRunning || serviceStatus === ServiceStatus.s_Stopping || serviceStatus === ServiceStatus.s_Starting){
                            scheme.objectModel.setData(TopologyModel.s_IconUrl_1, "Icons/Stopped", index);
                        }
                        else{
                            scheme.objectModel.setData(TopologyModel.s_IconUrl_1, "Icons/Alert", index);
                        }
                        if (index === scheme.selectedIndex){
                            topologyPage.commandsController.setCommandIsEnabled("Start", serviceStatus === ServiceStatus.s_NotRunning);
                            topologyPage.commandsController.setCommandIsEnabled("Stop", serviceStatus === ServiceStatus.s_Running);
                        }
                        let dependencyStatusModel = dataModel.getData(DependencyStatus.s_Key)
                        for (let i = 0; i < dependencyStatusModel.getItemsCount(); i++){
                            serviceId = dependencyStatusModel.getData("id", i);
                            index = scheme.findModelIndex(serviceId);
                            serviceStatus = scheme.objectModel.getData(ServiceStatus.s_Key, index);
                            dependencyStatus = dependencyStatusModel.getData(DependencyStatus.s_Key, i)
                            index = scheme.findModelIndex(serviceId);

                            if (dependencyStatus === DependencyStatus.s_NotRunning){
                                scheme.objectModel.setData(TopologyModel.s_IconUrl_2, "Icons/Error", index);
                            }
                            else if (dependencyStatus === DependencyStatus.s_Undefined) {
                                scheme.objectModel.setData(TopologyModel.s_IconUrl_2, "Icons/Warning", index);
                            }
                            else {
                                scheme.objectModel.setData(TopologyModel.s_IconUrl_2, "", index);
                            }

                            if (serviceStatus !== ServiceStatus.s_Running){
                                scheme.objectModel.setData(TopologyModel.s_IconUrl_2, "", index);
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
            inputParams.InsertField ("Item", scheme.objectModel.toJson());

            query.AddParam(inputParams);

            var queryFields = Gql.GqlObject("updatedNotification");
            queryFields.InsertField("id");
            query.AddField(queryFields);

            var gqlData = query.GetQuery();
            this.setGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                if (saveModel.containsKey("data")){
                    let dataModelLocal = saveModel.getData("data");
                    if (dataModelLocal.containsKey("SaveTopology")){
                        dataModelLocal = dataModelLocal.getData("SaveTopology");
                        if (dataModelLocal.containsKey("notification")){
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
            this.setGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                var dataModelLocal;
                if (topolodyModel.containsKey("errors")){
                    return;
                }

                console.log("GetTopology ready:", topolodyModel.toJson())

                if (topolodyModel.containsKey("data")){
                    dataModelLocal = topolodyModel.getData("data");
                    if (dataModelLocal.containsKey("GetTopology")){
                        dataModelLocal = dataModelLocal.getData("GetTopology");
                        if (dataModelLocal.containsKey("items")){
                            scheme.objectModel = dataModelLocal.getData("items");
                            scheme.requestPaint()
                            topologyPage.commandsController.setCommandIsEnabled("Save", false)
                        }
                    }
                }
            }
        }
    }
}
