import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import agentinoUpdatesSdl 1.0

// "Update" page (FixedItemQmlPath of UpdatePage, see Pages.acc): the service types installed on
// the agents with the number of agents using each of them (CServiceUpdateControllerComp).
// "Latest version" is not provided by the server yet, so the column stays empty.
ViewBase {
	id: updatePage

	anchors.fill: parent
	clip: true

	property bool loading: false
	// A change arrived while a request was in flight: reload once it is finished.
	property bool reloadPending: false

	function loadServices(){
		if (updatePage.loading){
			updatePage.reloadPending = true
			return
		}

		updatePage.loading = true
		updatePage.reloadPending = false
		serviceUpdatesRequestSender.send()
	}

	function finishLoading(){
		updatePage.loading = false
		if (updatePage.reloadPending){
			updatePage.loadServices()
		}
	}

	Component.onCompleted: {
		collectionTableStyle.apply(servicesTable)
		updatePage.loadServices()
	}

	AgentinoCollectionTableStyle {
		id: collectionTableStyle
	}

	TreeItemModel {
		id: headersModel

		Component.onCompleted: {
			let index = headersModel.insertNewItem()
			headersModel.setData("id", "name", index)
			headersModel.setData("name", qsTr("Name"), index)

			index = headersModel.insertNewItem()
			headersModel.setData("id", "agentCount", index)
			headersModel.setData("name", qsTr("Number of agents"), index)

			index = headersModel.insertNewItem()
			headersModel.setData("id", "latestVersion", index)
			headersModel.setData("name", qsTr("Latest version"), index)

			servicesTable.headers = headersModel
		}
	}

	Table {
		id: servicesTable

		objectName: "UpdateServicesTable"

		anchors.fill: parent

		readOnly: true
		hasSort: false
	}

	Text {
		anchors.centerIn: parent

		visible: !updatePage.loading && servicesTable.elementsCount === 0

		font.pixelSize: Style.fontSizeM
		font.family: Style.fontFamily
		color: Style.inactiveTextColor

		text: qsTr("No services")
	}

	GqlSdlRequestSender {
		id: serviceUpdatesRequestSender

		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "ServiceUpdatesList"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")

			let itemsFields = Gql.GqlObject("items")
			itemsFields.InsertField("id")
			itemsFields.InsertField("name")
			itemsFields.InsertField("agentCount")
			itemsFields.InsertField("latestVersion")
			payloadFields.InsertFieldObject(itemsFields)

			return payloadFields
		}

		sdlObjectComp: Component {
			ServiceUpdateListPayload {
				onFinished: {
					servicesTable.elements = hasItems() ? m_items : null
					updatePage.finishLoading()
				}
			}
		}

		// status -1 = error: release the single-request gate so later loads are sent.
		onFinished: {
			if (status < 0){
				updatePage.finishLoading()
			}
		}
	}

	// Services are added/removed on the agents and agents come and go: the counts change.
	SubscriptionClient {
		gqlCommandId: "OnServicesCollectionChanged"

		onMessageReceived: {
			updatePage.loadServices()
		}
	}

	SubscriptionClient {
		gqlCommandId: "OnAgentsCollectionChanged"

		onMessageReceived: {
			updatePage.loadServices()
		}
	}
}
