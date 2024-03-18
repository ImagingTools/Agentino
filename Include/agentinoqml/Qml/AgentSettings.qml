import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0
import imtguigql 1.0

ViewBase {
    id: agentSettingsContainer;

    property int mainMargin: 0;
    property int panelWidth: 700;

    Component.onCompleted: {
        settingsGetModel.updateModel()
    }

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: "AgentSettings";
    }

    commandsDelegate: ViewCommandsDelegateBase {
        id: settingsCommandsDelegate
        view: agentSettingsContainer;

        function commandHandle(commandId){
            console.log("onCommandActivated", commandId)
            if (commandId === "Save"){
                saveModel.save()
            }
        }
    }

    Connections {
        target: agentSettingsContainer.model;

        function onDataChanged(){
            commandsRepresentationProvider.setCommandIsEnabled("Save", true);
        }
    }

    function updateGui(){
        if (agentSettingsContainer.model.ContainsKey("Url")){
            agentinoUrlInput.text = agentSettingsContainer.model.GetData("Url");
        }
        else{
            agentinoUrlInput.text = "";
        }
    }

    function updateModel(){
        agentSettingsContainer.model.SetData("Url", agentinoUrlInput.text);
    }

    Rectangle {
        id: background;

        anchors.fill: parent;

        color: Style.backgroundColor;

        Item {
            id: columnContainer;

            width: agentSettingsContainer.panelWidth
            height: parent.height

            Column {
                id: bodyColumn;

                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.topMargin: Style.size_mainMargin;

                width: agentSettingsContainer.panelWidth - 2*anchors.leftMargin;

                spacing: Style.size_mainMargin;

                TextFieldWithTitle {
                    id: agentinoUrlInput;

                    width: parent.width

                    title: qsTr("Agentino URL");

                    placeHolderText: qsTr("Enter the agentino URL");

                    onEditingFinished: {
                        agentSettingsContainer.doUpdateModel();
                    }
                }
            }//Column bodyColumn
        }//columnContainer
    }

    property GqlModel settingsSaveModel: GqlModel {
        id: saveModel

        function save(){
            var query = Gql.GqlRequest("mutation", "SetAgentSettings");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField ("Item", agentSettingsContainer.model.toJSON());
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
                    if (dataModelLocal.ContainsKey("SetAgentSettings")){
                        dataModelLocal = dataModelLocal.GetData("SetAgentSettings");
                        if (dataModelLocal.ContainsKey("updatedNotification")){
                            commandsRepresentationProvider.setCommandIsEnabled("Save", false)
                        }
                    }
                }

            }
        }
    }

    property GqlModel settingsGetModel: GqlModel {
        id: settingsModel

        function updateModel() {
            console.log( "topologyPage updateModel", "GetAgentSettings");
            var query = Gql.GqlRequest("query", "GetAgentSettings");
            var queryFields = Gql.GqlObject("input");
            query.AddParam(queryFields);

            var gqlData = query.GetQuery();
            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            if (this.state === "Ready"){
                console.log("GetAgentSettings Ready:", this.toJSON());

                var dataModelLocal;
                if (this.ContainsKey("errors")){
                    return;
                }

                if (this.ContainsKey("data")){
                    dataModelLocal = this.GetData("data");
                    if (dataModelLocal.ContainsKey("GetAgentSettings")){
                        dataModelLocal = dataModelLocal.GetData("GetAgentSettings");

                        agentSettingsContainer.model = dataModelLocal;

                        agentSettingsContainer.doUpdateGui();
                    }
                }
            }
        }
    }

}//Container
