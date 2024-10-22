import QtQuick 2.0
import Acf 1.0

Item {
    id: window;

    anchors.fill: parent;

    Component {
        id: topRightPanelDecoratorComp;
        AgentTopRightPanelDecorator {}
    }

    AgentinoMain {
        id: application;

        anchors.fill: parent;

        serverReady: true

        Component.onCompleted: {
            designProvider.applyDesignSchema("Light");
            context.appName = "Agent"

            application.firstModelsInit();

            Style.topRightPanelDecorator = topRightPanelDecoratorComp;
        }

        function getServerUrl(){
            return context.location;
        }
    }
}
