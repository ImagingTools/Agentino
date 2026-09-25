import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0

Window {
	id: window;

	anchors.fill: parent;
	title: qsTr("Agentino")

	AgentinoMain {
		id: application;

		anchors.fill: parent;

		serverReady: true
		useWebSocketProxy: true

		Component.onCompleted: {
			designProvider.setDesignSchema("Light");
			context.application = ["ImtCore", "Agentino"];
		}

		function getServerUrl(){
			return context.location;
		}
	}
}
