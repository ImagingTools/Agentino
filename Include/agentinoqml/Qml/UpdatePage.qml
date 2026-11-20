import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import agentinoUpdatesSdl 1.0

// "Update" page (FixedItemQmlPath of UpdatePage, see Pages.acc): the service types installed on
// the agents with the number of agents using each of them (CServiceUpdateControllerComp).
// The panel on the left filters the table by the operating system family of the agents
// (Windows / Linux / macOS / Other).
// "Latest version" and "Auto-update" are not provided by the server yet, so the columns stay empty.
// A double click on a service opens the versions page of the service (ServiceVersionsView.qml) in a new tab.
ViewBase {
	id: updatePage

	anchors.fill: parent
	clip: true

	// Document service of the page (MultiDocPage "Update"): the versions pages are its documents.
	property var documentManager: MainDocumentService.getDocumentService("Update")
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

	// Opens the versions page of the service of the table row (the service type ID is the document ID).
	function openServiceVersions(index){
		if (!updatePage.documentManager || !servicesTable.elements || index < 0){
			return
		}

		let row = servicesTable.elements.get(index)
		let item = row ? row.item : null
		if (!item || !item.m_id){
			return
		}

		updatePage.documentManager.openDocument(item.m_id, "ServiceVersions", item.m_name)
	}

	Component.onCompleted: {
		if (updatePage.documentManager){
			updatePage.documentManager.registerDocumentView("ServiceVersions", serviceVersionsViewComp)
			updatePage.documentManager.registerDocumentDataController("ServiceVersions", serviceVersionsDataControllerComp)
		}

		collectionTableStyle.apply(servicesTable)
		updatePage.loadOperatingSystems()
		updatePage.loadServices()
	}

	Component {
		id: serviceVersionsViewComp

		ServiceVersionsView {}
	}

	// The versions page edits nothing: an empty model is enough for the document service.
	Component {
		id: serviceVersionsDataControllerComp

		DocumentDataController {
			documentModelComp: Component {
				TreeItemModel {}
			}

			function updateDocumentModel(){
				createDocumentModel()
			}
		}
	}

	AgentinoCollectionTableStyle {
		id: collectionTableStyle
	}

	// Operating systems of the agents: styled as the page list of MultiPageView (General / Services /
	// Agent Log / Terminal of AgentEditor) - no background of its own, a thin separator on the right.
	Item {
		id: operatingSystemsPanel

		objectName: "UpdateOperatingSystemsPanel"

		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		width: Style.sizeHintXXS + 2 * Style.marginM + operatingSystemsSeparator.width

		Item {
			id: operatingSystemsContent

			anchors.top: parent.top
			anchors.topMargin: Style.marginM
			anchors.left: parent.left
			anchors.leftMargin: Style.marginM
			anchors.bottom: parent.bottom

			width: Style.sizeHintXXS

			BaseText {
				id: operatingSystemsTitle

				anchors.top: parent.top
				anchors.left: parent.left
				anchors.leftMargin: Style.marginM
				anchors.right: parent.right
				anchors.rightMargin: Style.marginM

				height: Style.controlHeightM

				text: qsTr("Operating System")
				color: Style.subtitleColor
				font.family: Style.fontFamilyBold
				font.pixelSize: Style.fontSizeM
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
			}

			// Items that do not fit the height scroll instead of being cut off.
			Flickable {
				id: operatingSystemsFlickable

				anchors.top: operatingSystemsTitle.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.bottom: parent.bottom

				contentWidth: width
				contentHeight: operatingSystemsColumn.height + 2 * Style.marginS
				boundsBehavior: Flickable.StopAtBounds
				interactive: contentHeight > height
				clip: true

				Column {
					id: operatingSystemsColumn

					y: Style.marginS
					width: operatingSystemsFlickable.width
						   - (operatingSystemsScrollbar.visible ? operatingSystemsScrollbar.width + Style.marginXS : 0)

					spacing: Style.spacingXS

					Repeater {
						model: updatePage.operatingSystemsModel

						delegate: SidePanelItem {
							objectName: model.key === "all" ? "UpdateOperatingSystemAllButton"
									  : model.key === "unknown" ? "UpdateOperatingSystemUnknownButton"
									  : "UpdateOperatingSystemButton"

							width: operatingSystemsColumn.width

							text: model.name
							icon: model.icon
							selected: model.key === updatePage.selectedOsKey

							onClicked: {
								updatePage.selectOperatingSystem(model.key)
							}
						}
					}
				}
			}

			CustomScrollbar {
				id: operatingSystemsScrollbar

				anchors.right: parent.right
				anchors.top: operatingSystemsFlickable.top
				anchors.bottom: operatingSystemsFlickable.bottom

				secondSize: Style.marginS
				targetItem: operatingSystemsFlickable
			}
		}

		Rectangle {
			id: operatingSystemsSeparator

			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right

			width: 1
			color: Style.borderColor
			opacity: 0.5
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

			index = headersModel.insertNewItem()
			headersModel.setData("id", "autoUpdate", index)
			headersModel.setData("name", qsTr("Auto-update"), index)

			servicesTable.headers = headersModel
		}
	}


	// "Auto-update" cell: the StateOk icon for true, nothing for false or no value.
	Component {
		id: autoUpdateCellComp

		TableCellDelegateBase {
			id: autoUpdateCell

			property bool autoUpdate: false

			Image {
				anchors.left: parent.left
				anchors.leftMargin: Style.marginL
				anchors.verticalCenter: parent.verticalCenter

				width: Style.iconSizeS
				height: width
				fillMode: Image.PreserveAspectFit
				sourceSize.width: width
				sourceSize.height: height

				visible: autoUpdateCell.autoUpdate
				source: autoUpdateCell.autoUpdate ? "qrc:/" + Style.getIconPath("Icons/StateOk", Icon.State.On, Icon.Mode.Normal) : ""
			}

			onReused: {
				autoUpdateCell.autoUpdate = autoUpdateCell.getValue() === true
			}
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

		onHeadersChanged: {
			servicesTable.setColumnContentById("autoUpdate", autoUpdateCellComp)
		}

		onDoubleClicked: {
			updatePage.openServiceVersions(index)
		}
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
			itemsFields.InsertField("autoUpdate")
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
