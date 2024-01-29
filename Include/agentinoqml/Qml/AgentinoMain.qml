import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0

ApplicationMain{
    id: window;
    useWebSocketSubscription: true
    applicationId: "Agentino"

    ModalDialogManager {
        id: modalDialogManager;

        z: 30;

        anchors.fill: parent;
    }
}

