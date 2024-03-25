import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0

ApplicationMain{
    id: window;
    useWebSocketSubscription: true
    applicationId: "Agentino"

    canRecoveryPassword: false;

    ModalDialogManager {
        id: modalDialogManager;

        z: 30;

        anchors.fill: parent;
    }

    Component.onCompleted: {
        context.appName = "Agentino"
    }

    function startSystemStatusChecking(){
        console.log("startSystemStatusChecking")

        systemStatusController.serverStatusGqlCommandId = "AgentinoTestConnection"
        // systemStatusController.databaseStatusGqlCommandId = "ProLifeGetDatabaseStatus"
        systemStatusController.serverName = "Agentino"
        // systemStatusController.slaveSystemStatusController = pumaSystemStatusController;

        systemStatusController.updateSystemStatus();
        // firstModelsInit()
    }
}

