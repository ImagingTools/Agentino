import QtQuick 2.12
import Acf 1.0
//import imtdocgui 1.0
//import imtcolgui 1.0
import imtcontrols 1.0
//import imtguigql 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0


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

    Component.onCompleted: {
        // itemsTopologyModel.updateModel()
    }

    onVisibleChanged: {
        if (topologyPage.visible){
            itemsTopologyModel.updateModel()
        }
    }

    commandsDelegate: Item {
        function commandHandle(commandId) {
            if (commandId === "Save"){
                topologySaveModel.save()
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
        }
    }

    property GqlModel topologySaveModel: GqlModel {
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

                console.log("GetTopology ready:" )

                if (topolodyModel.ContainsKey("data")){
                    dataModelLocal = topolodyModel.GetData("data");
                    if (dataModelLocal.ContainsKey("GetTopology")){
                        dataModelLocal = dataModelLocal.GetData("GetTopology");
                        if (dataModelLocal.ContainsKey("items")){
                            scheme.objectModel = dataModelLocal.GetData("items");
                            scheme.requestPaint()
                        }
                    }
                }
            }
            else if (this.state === "Error"){
            }
        }
    }

}
