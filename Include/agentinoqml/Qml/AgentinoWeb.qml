import QtQuick 2.0
import Acf 1.0

Item {
    id: window;

    anchors.fill: parent;

    AgentinoMain {
        id: application;

        anchors.fill: parent;

        serverReady: true

        Component.onCompleted: {
            designProvider.applyDesignSchema("Light");
            context.appName = "Agentino"
            context.application = ["ImtCore", "Agentino"];
        }

        function getServerUrl(){
            return context.location;
        }
    }
}
