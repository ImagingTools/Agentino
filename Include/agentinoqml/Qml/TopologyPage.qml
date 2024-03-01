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
    property real contentWidth: mainContainer.width.toFixed(3);
    property real contentHeight: mainContainer.height.toFixed(3);
    property real contentX: -mainContainer.x.toFixed(3);
    property real contentY: -mainContainer.y.toFixed(3);
    property real originX: 0;
    property real originY: 0;
    //for scrollBars

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: "Topology"
        uuid: topologyPage.viewId;
    }

    onVisibleChanged: {
        if (topologyPage.visible){
            itemsTopologyModel.updateModel()
        }
    }

    commandsDelegate: Item {
        function commandHandle(commandId) {
            if (commandId === "Save"){
                saveModel.save()
            }
        }
    }

    onContentXChanged: {
        if(mainContainer.x !== - contentX){
            mainContainer.x = - contentX;
        }
    }
    onContentYChanged: {
        if(mainContainer.y !== - contentY){
            mainContainer.y = - contentY;
        }
    }

    SchemeView {
        id: scheme

        Component.onCompleted: {
            let documentManager = MainDocumentManager.getDocumentManager("Topology");
            if (documentManager){
                documentManager.registerDocumentView("Service", "ServiceView", serviceEditorComp);
                documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
            }
        }

        onModelDataChanged: {
            commandsRepresentationProvider.setCommandIsEnabled("Save", true)
        }

        function goInside(){
            let documentManager = MainDocumentManager.getDocumentManager("Topology");
            if (documentManager){
                if(objectModel.GetItemsCount() > selectedIndex && selectedIndex >= 0){
                    let serviceId = objectModel.GetData("Id", selectedIndex);
                    console.log("Go inside");

                    documentManager.openDocument(serviceId, "Service", "ServiceView");
                }
            }
        }
    }

    function getAdditionalInputParams(){
        console.log("getAdditionalInputParams");
        let additionInputParams = {}

        console.log("scheme.selectedIndex", scheme.selectedIndex);

        if(scheme.selectedIndex >= 0){
            let agentId = scheme.objectModel.GetData("AgentId", scheme.selectedIndex);
            additionInputParams["clientId"] = agentId;

            console.log("additionInputParams", JSON.stringify(additionInputParams));
        }

        return additionInputParams
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

//            function getDocumentName() {
//                let newName = qsTr("<New service>");

//                if (documentName !== ""){
//                    return documentName + "@" + root.clientName
//                }

//                return newName + "@" + root.clientName
//            }
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
