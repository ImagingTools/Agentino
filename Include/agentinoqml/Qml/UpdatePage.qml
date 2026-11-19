import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import agentinoUpdatesSdl 1.0

// "Update" page (FixedItemQmlPath of UpdatePage, see Pages.acc): the service types installed on
// the agents with the number of agents using each of them (CServiceUpdateControllerComp).
// The panel on the left filters the table by the operating system family of the agents
// (Windows / Linux / macOS / Other).
// "Latest version" is not provided by the server yet, so the column stays empty.
ViewBase {
	id: updatePage

	anchors.fill: parent
	clip: true

	// Selected item of the operating system panel: "all", "unknown" (agents that did not report
	// their operating system) or "os:" + the operating system family.
	property string selectedOsKey: "all"
	// Items: {key, name, icon}
	property ListModel operatingSystemsModel: ListModel {}

	property bool loading: false
	// A change arrived while a request was in flight: reload once it is finished.
	property bool reloadPending: false

	function selectOperatingSystem(key){
		if (updatePage.selectedOsKey === key){
			return
		}

		updatePage.selectedOsKey = key
		updatePage.loadServices()
	}

	function loadOperatingSystems(){
		operatingSystemsRequestSender.send()
	}

	function setOperatingSystems(names, hasUnknown){
		updatePage.operatingSystemsModel.clear()
		updatePage.operatingSystemsModel.append({"key": "all", "name": qsTr("All"), "icon": "Icons/Agents"})

		let selectionFound = updatePage.selectedOsKey === "all"
		for (let i = 0; i < names.length; ++i){
			let key = "os:" + names[i]
			// Families come from the server: "Windows", "Linux", "macOS" are names, "Other" is translated.
			let name = names[i] === "Other" ? qsTr("Other") : names[i]
			updatePage.operatingSystemsModel.append({"key": key, "name": name, "icon": "Icons/System"})
			if (key === updatePage.selectedOsKey){
				selectionFound = true
			}
		}

		if (hasUnknown){
			updatePage.operatingSystemsModel.append({"key": "unknown", "name": qsTr("Unknown"), "icon": "Icons/System"})
			if (updatePage.selectedOsKey === "unknown"){
				selectionFound = true
			}
		}

		// No agent of the selected operating system is left.
		if (!selectionFound){
			updatePage.selectOperatingSystem("all")
		}
	}

	function loadServices(){
		if (updatePage.loading){
			updatePage.reloadPending = true
			return
		}

		updatePage.loading = true
		updatePage.reloadPending = false

		// No "operatingSystem" for all agents; an empty one for the agents of unknown operating system.
		serviceUpdatesRequestSender.inputParams = Gql.GqlObject("input")
		if (updatePage.selectedOsKey === "unknown"){
			serviceUpdatesRequestSender.addInputParam("operatingSystem", "")
		}
		else if (updatePage.selectedOsKey.indexOf("os:") === 0){
			serviceUpdatesRequestSender.addInputParam("operatingSystem", updatePage.selectedOsKey.substring(3))
		}

		serviceUpdatesRequestSender.requestedOsKey = updatePage.selectedOsKey
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
		updatePage.loadOperatingSystems()
		updatePage.loadServices()
	}

	AgentinoCollectionTableStyle {
		id: collectionTableStyle
	}

	// Operating systems of the agents: styled as the main menu of the application.
	Rectangle {
		id: operatingSystemsPanel

		objectName: "UpdateOperatingSystemsPanel"

		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		width: Style.sizeHintXXS
		color: Style.menuPanelBackgroundColor

		Rectangle {
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			width: 1
			color: Style.borderColor
		}

		BaseText {
			id: operatingSystemsTitle

			anchors.top: parent.top
			anchors.topMargin: Style.marginS
			anchors.left: parent.left
			anchors.leftMargin: Style.marginL
			anchors.right: parent.right
			anchors.rightMargin: Style.marginS

			height: Style.controlHeightM

			text: qsTr("Operating System")
			color: Style.subtitleColor
			font.family: Style.fontFamilyBold
			font.pixelSize: Style.fontSizeM
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
		}

		ListView {
			id: operatingSystemsList

			anchors.top: operatingSystemsTitle.bottom
			anchors.topMargin: Style.marginS
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			anchors.bottomMargin: Style.marginS

			clip: true
			boundsBehavior: Flickable.StopAtBounds
			model: updatePage.operatingSystemsModel

			delegate: MenuPanelButton {
				objectName: model.key === "all" ? "UpdateOperatingSystemAllButton"
						  : model.key === "unknown" ? "UpdateOperatingSystemUnknownButton"
						  : "UpdateOperatingSystemButton"

				width: operatingSystemsList.width
				height: Style.controlHeightL

				text: model.name
				textColor: Style.textColor
				fontName: Style.fontFamily
				selected: model.key === updatePage.selectedOsKey
				iconSource: "qrc:/" + Style.getIconPath(model.icon, Icon.State.On,
							(highlighted || selected) ? Icon.Mode.Selected : Icon.Mode.Normal)

				onClicked: {
					updatePage.selectOperatingSystem(model.key)
				}
			}
		}

		// Shown only when the list does not fit.
		CustomScrollbar {
			z: operatingSystemsList.z + 1
			anchors.right: operatingSystemsList.right
			anchors.rightMargin: Style.marginXS
			anchors.top: operatingSystemsList.top
			anchors.bottom: operatingSystemsList.bottom
			secondSize: Style.marginS
			radius: Style.radiusS
			targetItem: operatingSystemsList
		}
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

		anchors.left: operatingSystemsPanel.right
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		readOnly: true
		hasSort: false
	}

	Text {
		anchors.centerIn: servicesTable

		visible: !updatePage.loading && servicesTable.elementsCount === 0

		font.pixelSize: Style.fontSizeM
		font.family: Style.fontFamily
		color: Style.inactiveTextColor

		text: qsTr("No services")
	}

	GqlSdlRequestSender {
		id: operatingSystemsRequestSender

		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "AgentOperatingSystemsList"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")
			payloadFields.InsertField("names")
			payloadFields.InsertField("hasUnknown")

			return payloadFields
		}

		sdlObjectComp: Component {
			AgentOperatingSystemList {
				onFinished: {
					let names = []
					if (m_names){
						let count = m_names.length !== undefined ? m_names.length : 0
						for (let i = 0; i < count; ++i){
							if (m_names[i] !== undefined && m_names[i] !== null){
								names.push("" + m_names[i])
							}
						}
					}

					updatePage.setOperatingSystems(names, m_hasUnknown === true)
				}
			}
		}
	}

	GqlSdlRequestSender {
		id: serviceUpdatesRequestSender

		// Operating system filter of the request in flight; the response is dropped if another one was chosen meanwhile.
		property string requestedOsKey: ""

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
					if (serviceUpdatesRequestSender.requestedOsKey === updatePage.selectedOsKey){
						servicesTable.elements = hasItems() ? m_items : null
					}
					else{
						updatePage.reloadPending = true
					}

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

	// Services are added/removed on the agents: the counts change.
	SubscriptionClient {
		gqlCommandId: "OnServicesCollectionChanged"

		onMessageReceived: {
			updatePage.loadServices()
		}
	}

	// Agents come and go (and report their operating system): both the panel and the counts change.
	SubscriptionClient {
		gqlCommandId: "OnAgentsCollectionChanged"

		onMessageReceived: {
			updatePage.loadOperatingSystems()
			updatePage.loadServices()
		}
	}
}
