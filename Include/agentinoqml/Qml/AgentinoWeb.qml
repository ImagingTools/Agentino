import QtQuick 2.0
import Acf 1.0

Item {
    id: window;

    anchors.fill: parent;

    AgentinoMain {
        id: application;

        anchors.fill: parent;

        systemStatus: "NO_ERROR";

        Component.onCompleted: {
            designProvider.applyDesignSchema("Light");

            application.firstModelsInit();
        }
    }
}
