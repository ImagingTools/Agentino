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
        viewId = "AgentSettings"
        settingsModel.updateModel()
    }

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: agentSettingsContainer.viewId;
//        uuid: agentSettingsContainer.viewId;
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

    function blockEditing(){
        descriptionInput.readOnly = true;
        nameInput.readOnly = true;
    }

    function updateGui(){
//        if (agentSettingsContainer.model.ContainsKey("Name")){
//            nameInput.text = agentSettingsContainer.model.GetData("Name");
//        }
//        else{
//            nameInput.text = "";
//        }

//        if (agentSettingsContainer.model.ContainsKey("Description")){
//            descriptionInput.text = agentSettingsContainer.model.GetData("Description");
//        }
//        else{
//            descriptionInput.text = "";
//        }

        if (agentSettingsContainer.model.ContainsKey("agentinoUrl")){
            agentinoUrlInput.text = agentSettingsContainer.model.GetData("agentinoUrl");
        }
        else{
            agentinoUrlInput.text = "";
        }
    }

    function updateModel(){
        console.log("updateModel()")
//        agentSettingsContainer.model.SetData("Name", nameInput.text);
//        agentSettingsContainer.model.SetData("Description", descriptionInput.text);
        agentSettingsContainer.model.SetData("agentinoUrl", agentinoUrlInput.text);
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

//                TextFieldWithTitle {
//                    id: nameInput;

//                    width: parent.width

//                    title: qsTr("Name");

//                    placeHolderText: qsTr("Enter the name");

//                    onEditingFinished: {
//                        agentSettingsContainer.doUpdateModel();
//                    }

//                    KeyNavigation.tab: nameInput;
//                }

//                TextFieldWithTitle {
//                    id: descriptionInput;

//                    width: parent.width

//                    title: qsTr("Description");

//                    placeHolderText: qsTr("Enter the descriptionl");

//                    onEditingFinished: {
//                        agentSettingsContainer.doUpdateModel();
//                    }

//                    KeyNavigation.tab: nameInput;
//                }

                TextFieldWithTitle {
                    id: agentinoUrlInput;

                    width: parent.width

                    title: qsTr("Agentino Web Socket Url");

                    placeHolderText: qsTr("Enter the agentino Url");

                    onEditingFinished: {
                        agentSettingsContainer.doUpdateModel();
                    }

                    KeyNavigation.tab: nameInput;
                }
            }//Column bodyColumn
        }//columnContainer
    }

    property GqlModel settingsSaveModel: GqlModel {
        id: saveModel

        function save(){
            var query = Gql.GqlRequest("mutation", "SetAgentSettings");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField ("agentinoUrl", agentinoUrlInput.text);

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
                        if (dataModelLocal.ContainsKey("updateNotification")){
//                            commandsRepresentationProvider.setCommandIsEnabled("Save", false)
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
            var queryFields = Gql.GqlObject("settings");

            query.AddField(queryFields);

            var gqlData = query.GetQuery();

            console.log("settingsGetModel query ", gqlData);
            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state);
            if (this.state === "Ready"){
                var dataModelLocal;
                if (settingsModel.ContainsKey("errors")){
                    return;
                }

                console.log("GetTopology ready:", settingsModel.toJSON())

                if (settingsModel.ContainsKey("data")){
                    dataModelLocal = settingsModel.GetData("data");
                    if (dataModelLocal.ContainsKey("GetAgentSettings")){
                        dataModelLocal = dataModelLocal.GetData("GetAgentSettings");
                        if (dataModelLocal.ContainsKey("settings")){
                            agentinoUrlInput.text = dataModelLocal.GetData("settings").GetData("agentinoUrl");
//                            commandsRepresentationProvider.setCommandIsEnabled("Save", false)
                        }
                    }
                }
            }
            else if (this.state === "Error"){
            }
        }
    }

}//Container
