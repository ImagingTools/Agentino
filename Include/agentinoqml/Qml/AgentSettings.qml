import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0
import imtguigql 1.0

ViewBase {
    id: agentSettingsContainer;

    property int mainMargin: 0;
    property int panelWidth: 700;

    property string agentStatus: "Unknown";

    Component.onCompleted: {
        settingsGetModel.updateModel()
    }

    onAgentStatusChanged: {
        console.log("onAgentStatusChanged", agentStatus);
        loading.visible = false;
        //        statusIcon.source = "";
        statusText.text = "";
        button.enabled = false;

        if (agentStatus == "Connected"){
            statusIcon.color = "green";
            statusText.text = qsTr("Connected");
        }
        else if (agentStatus == "Disconnected"){
            statusIcon.color = "red";
            statusText.text = qsTr("Disconnected");

            button.enabled = true;
        }
        else if (agentStatus == "Checking"){
            loading.visible = true;
            statusText.text = qsTr("Checking the connection")
        }
        else{
            statusIcon.color = "yellow";
            statusText.text = qsTr("Unknown");

            button.enabled = true;
        }
    }

    Connections {
        target: agentSettingsContainer.model;

        function onDataChanged(){
            button.enabled = true;
        }
    }

    Connections {
        target: subscriptionManager;

        function onStatusChanged(){
            if (subscriptionManager.status == WebSocket.Error ||
                    subscriptionManager.status == WebSocket.Closed){
                agentSettingsContainer.agentStatus = "Disconnected";
            }
        }
    }

    function updateGui(){
        if (agentSettingsContainer.model.containsKey("Url")){
            agentinoUrlInput.text = agentSettingsContainer.model.getData("Url");
        }
        else{
            agentinoUrlInput.text = "";
        }
    }

    function updateModel(){
        agentSettingsContainer.model.setData("Url", agentinoUrlInput.text);
    }

    Rectangle {
        id: background;

        anchors.fill: parent;

        color: Style.imagingToolsGradient0;

        Rectangle {
            id: columnContainer;

            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.verticalCenter: parent.verticalCenter;

            width: agentSettingsContainer.panelWidth
            height: 300;

            color: Style.backgroundColor;

            Column {
                id: bodyColumn;

                anchors.fill: parent;
                anchors.margins: Style.size_mainMargin;

                spacing: Style.size_mainMargin;

                Item {
                    width: parent.width;
                    height: 30;

                    Text {
                        id: title;

                        anchors.verticalCenter: parent.verticalCenter;

                        text: qsTr("Status");
                        color: Style.textColor;
                        font.family: Style.fontFamilyBold;
                        font.pixelSize: Style.fontSize_subtitle;
                    }

                    Loading {
                        id: loading;

                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.left: title.right;
                        anchors.leftMargin: Style.size_smallMargin

                        width: 20;
                        height: width;

                        indicatorSize: 20;

                        visible: true;
                    }

                    Rectangle {
                        id: statusIcon;

                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.left: title.right;
                        anchors.leftMargin: Style.size_smallMargin

                        width: 20;
                        height: width;

                        radius: width;

                        visible: !loading.visible;
                    }

                    Text {
                        id: statusText;

                        anchors.left: loading.visible ? loading.right : statusIcon.right;
                        anchors.leftMargin: Style.size_smallMargin
                        anchors.right: parent.right;
                        anchors.verticalCenter: parent.verticalCenter;

                        font.pixelSize: Style.fontSize_subtitle;
                        font.family: Style.fontFamily;
                        color: Style.textColor;

                        elide: Text.ElideRight;

                        text: qsTr("Checking the connection");
                    }
                }

                Item {
                    width: parent.width
                    height: agentinoUrlInput.height;

                    TextFieldWithTitle {
                        id: agentinoUrlInput;

                        width: parent.width -button.width - button.anchors.leftMargin;

                        title: qsTr("Agentino URL");
                        titleFontPixelSize: Style.fontSize_subtitle;
                        titleFontFamily: Style.fontFamilyBold;

                        placeHolderText: qsTr("Enter the agentino URL");

                        onEditingFinished: {
                            agentSettingsContainer.doUpdateModel();
                        }
                    }

                    Button {
                        id: button;

                        anchors.left: agentinoUrlInput.right;
                        anchors.leftMargin: Style.size_mainMargin;
                        anchors.bottom: parent.bottom;

                        width: 100;
                        height: 32;

                        text: qsTr("Connect")

                        enabled: false;

                        onClicked: {
                            agentSettingsContainer.agentStatus = "Checking";

                            saveModel.save()
                        }

                        decorator: Component {ButtonDecorator {
                            font.pixelSize: Style.fontSize_subtitle;
                        }}
                    }
                }
            }//Column bodyColumn
        }//columnContainer
    }

    SubscriptionClient {
        id: subscriptionClient;

        Component.onCompleted: {
            let subscriptionRequestId = "OnAgentConnectionChanged"
            var query = Gql.GqlRequest("subscription", subscriptionRequestId);
            var queryFields = Gql.GqlObject("notification");
            queryFields.InsertField("Id");
            query.AddField(queryFields);

            subscriptionManager.registerSubscription(query, subscriptionClient);
        }

        onStateChanged: {
            if (state === "Ready"){
                console.log("OnAgentConnectionChanged Ready", subscriptionClient.toJson());
                if (subscriptionClient.containsKey("data")){
                    let dataModel = subscriptionClient.getData("data");
                    if (dataModel.containsKey("OnAgentConnectionChanged")){
                        dataModel = dataModel.getData("OnAgentConnectionChanged");

                        if (dataModel.containsKey("status")){
                            let status = dataModel.getData("status");
                            agentSettingsContainer.agentStatus = status;
                        }
                    }
                }
            }
        }
    }

    property GqlModel settingsSaveModel: GqlModel {
        id: saveModel

        function save(){
            var query = Gql.GqlRequest("mutation", "SetAgentSettings");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField ("Item", agentSettingsContainer.model.toJson());
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
                if (saveModel.containsKey("data")){
                    let dataModelLocal = saveModel.getData("data");
                    if (dataModelLocal.containsKey("SetAgentSettings")){
                        dataModelLocal = dataModelLocal.getData("SetAgentSettings");
                        if (dataModelLocal.containsKey("updatedNotification")){
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
            var queryFields = Gql.GqlObject("input");
            query.AddParam(queryFields);

            var gqlData = query.GetQuery();
            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            if (this.state === "Ready"){
                console.log("GetAgentSettings Ready:", this.toJson());

                var dataModelLocal;
                if (this.containsKey("errors")){
                    return;
                }

                if (this.containsKey("data")){
                    dataModelLocal = this.getData("data");
                    if (dataModelLocal.containsKey("GetAgentSettings")){
                        dataModelLocal = dataModelLocal.getData("GetAgentSettings");

                        agentSettingsContainer.model = dataModelLocal;

                        agentSettingsContainer.doUpdateGui();
                    }
                }
            }
        }
    }

}//Container
