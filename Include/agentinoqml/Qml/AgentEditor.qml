import QtQuick 2.15
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentinoAgentsSdl 1.0

/**
	MultiPageView editor, by analogy with ServiceEditor: General is the agent's own fields,
	Services/Log are what used to be reachable only from AgentCollectionView (the standalone
	"Services" toolbar command and the collection page's split-view agent log) - moved here so
	both live with the agent they belong to, the same way ServiceEditor hosts its own Log page.
*/
DocumentViewBase {
	id: agentEditorContainer

	anchors.fill: parent
	contentColor: Style.baseColor

	property AgentData agentData: model

	// Services/Log need a real, persisted agent id - mirrors ServiceEditor's isNewService/
	// ensureLogPage() gating (documentManager.documentIsNew(), not m_id emptiness - the
	// document framework allocates a real id as soon as the document is created, well before
	// any Save).
	property bool isNewAgent: true

	function refreshIsNewAgent(){
		if (!documentManager || documentId === "")
			return
		if (typeof documentManager.documentIsNew !== "function")
			return
		isNewAgent = documentManager.documentIsNew(documentId)
	}

	function ensureServicesPage(){
		if (isNewAgent || multiPageView.getIndexById("Services") >= 0){
			return
		}
		multiPageView.addPage("Services", qsTr("Services"), servicesPageComp, "Icons/ServiceCollection")
		agentEditorContainer.updateServicesBadge()
	}

	function ensureLogPage(){
		if (isNewAgent || multiPageView.getIndexById("Log") >= 0){
			return
		}
		multiPageView.addPage("Log", qsTr("Agent Log"), agentLogPageComp, "Icons/EventLog")
	}

	// Was the "Terminal" toolbar command of AgentCollectionView (a fixed workspace tab
	// bound to the row selected at the time it was pressed) - now this agent's own page,
	// so the shell always belongs to the agent whose editor it is shown in. Needs a
	// persisted agent id like Services/Log, plus the RemoteTerminal permission: remote
	// command execution is the most sensitive capability in the product, and the command
	// this replaces carried the same permission.
	function ensureTerminalPage(){
		if (isNewAgent || multiPageView.getIndexById("Terminal") >= 0){
			return
		}
		if (!PermissionsController.checkPermission("RemoteTerminal")){
			return
		}
		multiPageView.addPage("Terminal", qsTr("Terminal"), terminalPageComp, "Icons/Terminal")
	}

	// "Services (N)" in the page list - N comes straight off the AgentData payload
	// (serviceCount, populated the same way the Agents grid's own "services" column is:
	// m_serviceManagerCompPtr->GetServiceCollection(agentId)), so it's known as soon as the
	// document loads, no separate query needed.
	function updateServicesBadge(){
		if (!agentEditorContainer.agentData){
			return
		}
		let count = agentEditorContainer.agentData.m_serviceCount
		if (count === undefined || count === null){
			return
		}
		multiPageView.setPageBadge("Services", "(" + count + ")")
	}

	onAgentDataChanged: agentEditorContainer.updateServicesBadge()

	// "New messages" marker for the Log page: the backend already proxies
	// OnAgentLogCollectionChanged end-to-end (agent -> server -> GUI clients, same path as
	// OnServiceLogCollectionChanged) but nothing subscribed to it client-side before. This
	// does not filter by agent id (the payload's exact field isn't confirmed) - same
	// coarse-grained "any change of this type refreshes/marks" convention this app already
	// uses for OnAgentStatusChanged - so it can mark the dot while looking at an unrelated
	// agent's editor tab; acceptable for a "something changed, go check" indicator.
	property bool hasNewLogMessages: false

	onHasNewLogMessagesChanged: {
		multiPageView.setPageBadge("Log", hasNewLogMessages ? "•" : "");
	}

	SubscriptionClient {
		id: agentLogSubscriptionClient
		gqlCommandId: "OnAgentLogCollectionChanged"

		onMessageReceived: {
			if (multiPageView.currentIndex !== multiPageView.getIndexById("Log")){
				agentEditorContainer.hasNewLogMessages = true;
			}
		}
	}

	Connections {
		target: multiPageView
		function onCurrentIndexChanged(){
			if (multiPageView.currentIndex >= 0 && multiPageView.currentIndex === multiPageView.getIndexById("Log")){
				agentEditorContainer.hasNewLogMessages = false;
			}
		}
	}

	onDocumentIdChanged: refreshIsNewAgent()
	onDocumentManagerChanged: refreshIsNewAgent()
	onIsNewAgentChanged: {
		if (!isNewAgent){
			ensureServicesPage()
			ensureLogPage()
			ensureTerminalPage()
		}
	}

	Connections {
		target: agentEditorContainer.documentManager
		function onDocumentSaved(savedDocumentId){
			if (savedDocumentId === agentEditorContainer.documentId){
				agentEditorContainer.refreshIsNewAgent()
				agentEditorContainer.ensureServicesPage()
				agentEditorContainer.ensureLogPage()
				agentEditorContainer.ensureTerminalPage()
			}
		}
		// openDocument marks isNew=false before the view gets documentId; once the model is
		// bound and documentOpened fires, re-sync so Services/Log show up on an existing
		// agent opened at first start (same reasoning as ServiceEditor's onDocumentOpened).
		function onDocumentOpened(openedDocumentId){
			if (openedDocumentId === agentEditorContainer.documentId){
				agentEditorContainer.refreshIsNewAgent()
				agentEditorContainer.ensureServicesPage()
				agentEditorContainer.ensureLogPage()
				agentEditorContainer.ensureTerminalPage()
			}
		}
		function onDocumentAdded(addedDocumentId){
			if (addedDocumentId === agentEditorContainer.documentId){
				agentEditorContainer.refreshIsNewAgent()
			}
		}
	}

	function updateGui(){
		let idx = multiPageView.getIndexById("General")
		if (idx >= 0){
			multiPageView.ensurePageLoaded(idx)
			let item = multiPageView.getPageByIndex(idx)
			if (item) item.updateGui()
		}
	}

	function updateModel(){
		let idx = multiPageView.getIndexById("General")
		if (idx >= 0){
			multiPageView.ensurePageLoaded(idx)
			let item = multiPageView.getPageByIndex(idx)
			if (item) item.updateModel()
		}
	}

	MultiPageView {
		id: multiPageView
		anchors.fill: parent
		panelWidth: Style.sizeHintXXS

		function addAgentPages(){
			multiPageView.clear()
			multiPageView.addPage("General", qsTr("General"), generalPageComp, "Icons/Settings")
			agentEditorContainer.ensureServicesPage()
			agentEditorContainer.ensureLogPage()
			agentEditorContainer.ensureTerminalPage()
			multiPageView.currentIndex = multiPageView.getIndexById("General")
		}
	}

	Component.onCompleted: {
		multiPageView.addAgentPages()
	}

	Component {
		id: generalPageComp

		Flickable {
			id: generalFlickable

			anchors.fill: parent
			anchors.leftMargin: Style.marginXL
			anchors.topMargin: Style.marginXL
			anchors.rightMargin: Style.marginXL
			anchors.bottomMargin: Style.marginXL

			contentWidth: bodyColumn.width
			contentHeight: bodyColumn.height + Style.marginXL

			boundsBehavior: Flickable.StopAtBounds
			clip: true

			function updateGui(){
				if (!agentEditorContainer.agentData){
					return
				}

				nameInput.text = agentEditorContainer.agentData.m_name
				descriptionInput.text = agentEditorContainer.agentData.m_description

				switchVerboseMessage.checked = (agentEditorContainer.agentData.m_tracingLevel > -1)

				if (agentEditorContainer.agentData.m_tracingLevel > -1){
					tracingLevelInput.currentIndex = agentEditorContainer.agentData.m_tracingLevel
				}
				else{
					tracingLevelInput.currentIndex = -1
				}
			}

			function updateModel(){
				if (!agentEditorContainer.agentData){
					return
				}

				agentEditorContainer.agentData.m_name = nameInput.text
				agentEditorContainer.agentData.m_description = descriptionInput.text

				if (switchVerboseMessage.checked){
					if (tracingLevelInput.currentIndex < 0){
						agentEditorContainer.agentData.m_tracingLevel = 0
					}
					else{
						agentEditorContainer.agentData.m_tracingLevel = tracingLevelInput.currentIndex
					}
				}
				else{
					agentEditorContainer.agentData.m_tracingLevel = -1
				}
			}

			Column {
				id: bodyColumn

				width: Math.max(0, Math.min(700, generalFlickable.width))

				spacing: 10

				Text {
					anchors.left: parent.left

					color: Style.textColor
					font.family: Style.fontFamily
					font.pixelSize: Style.fontSizeM

					text: qsTr("Name")
				}

				CustomTextField {
					id: nameInput

					width: parent.width
					height: Style.itemSizeM

					placeHolderText: qsTr("Enter the name")

					onEditingFinished: {
						agentEditorContainer.doUpdateModel()
					}

					KeyNavigation.tab: descriptionInput
				}

				Text {
					anchors.left: parent.left

					color: Style.textColor
					font.family: Style.fontFamily
					font.pixelSize: Style.fontSizeM

					text: qsTr("Description")
				}

				CustomTextField {
					id: descriptionInput

					width: parent.width
					height: Style.itemSizeM

					placeHolderText: qsTr("Enter the description")

					onEditingFinished: {
						agentEditorContainer.doUpdateModel()
					}

					KeyNavigation.tab: nameInput
				}

				Text {
					anchors.left: parent.left
					width: parent.width

					color: Style.textColor
					font.family: Style.fontFamily
					font.pixelSize: Style.fontSizeM

					text: qsTr("Verbose message (") + (switchVerboseMessage.checked ? qsTr("on") : qsTr("off")) + ")"
				}

				Row {
					height: Style.itemSizeM
					spacing: Style.marginM

					SwitchCustom {
						id: switchVerboseMessage
						anchors.verticalCenter: parent.verticalCenter

						backgroundColor: "#D4D4D4"
						onCheckedChanged: {
							agentEditorContainer.doUpdateModel()
						}
					}

					Item {
						width: Style.marginM
						height: Style.itemSizeM
					}

					Text {
						anchors.verticalCenter: parent.verticalCenter

						visible: switchVerboseMessage.checked
						color: Style.textColor
						font.family: Style.fontFamily
						font.pixelSize: Style.fontSizeM

						text: qsTr("Tracing level")
					}

					ComboBox {
						id: tracingLevelInput
						anchors.verticalCenter: parent.verticalCenter
						height: Style.itemSizeM * 0.75
						width: Style.itemSizeL
						visible: switchVerboseMessage.checked

						model: TreeItemModel {}

						Component.onCompleted: {
							for (let i = 0; i <= 5; i++){
								let index = tracingLevelInput.model.insertNewItem()
								tracingLevelInput.model.setData("id", "" + i, index)
								tracingLevelInput.model.setData("name", "" + i, index)
							}
						}
						onCurrentIndexChanged: {
							agentEditorContainer.doUpdateModel()
						}
					}
				}
			}
		}
	}

	// Was AgentCollectionViewBase.qml's "Services" toolbar command (open a fixed-tab
	// workspace showing the selected agent's services) - now this agent's own page.
	// MultiDoc: ServiceCollectionView is a pinned Fixed first tab (cannot be closed);
	// service editors open in additional closable tabs.
	Component {
		id: servicesPageComp

		MultiDocWorkspaceView {
			id: servicesWorkspaceView
			anchors.fill: parent
			// Local only: page + tab strip on baseColor so Services matches AgentEditor.
			// Tab chrome is ServicesDocumentTabDecorator — global TabPanelDecorator is untouched.
			contentColor: Style.baseColor
			tabPanelColor: Style.baseColor
			tabDelegateDecorator: servicesTabDecoratorComp
			// Default icon for individual service document tabs opened from this workspace.
			defaultDocumentIcon: "Icons/Service"
			documentManager: DocumentService {}

			visualStatusProvider: GqlBasedObjectVisualStatusProvider {
				collectionId: "Services"
				function getHeaders(){
					return {"clientid": agentEditorContainer.documentId}
				}
			}

			Component.onCompleted: {
				MainDocumentService.registerDocumentService("Services", documentManager)
				// Fixed / pinned collection tab — close button is hidden by TabPanel.
				addFixedView(
							serviceCollectionViewComp,
							qsTr("Services"),
							"ServiceCollection",
							true,
							true,
							"Icons/ServiceCollection")
			}

			Component {
				id: servicesTabDecoratorComp
				ServicesDocumentTabDecorator {}
			}

			Component {
				id: serviceCollectionViewComp

				ServiceCollectionView {
					id: serviceCollectionView

					Component.onCompleted: {
						clientId = agentEditorContainer.documentId
					}

					// Live row count of the loaded Services list - unlike agentData.m_serviceCount
					// (a one-shot server snapshot taken when the agent representation loaded), this
					// tracks add/remove of services while this page is open.
					onElementsCountChanged: {
						multiPageView.setPageBadge("Services", "(" + serviceCollectionView.elementsCount + ")")
					}
				}
			}
		}
	}

	// Was AgentCollectionView.qml's second SplitView pane (a MessageCollectionView scoped to
	// the selected row via clientid) - now this agent's own Log page, matching ServiceEditor's
	// serviceLogPageComp (no services-name filter here - AgentData carries no service list,
	// and ServiceEditor's own Log page has no comparable extra filter either).
	Component {
		id: agentLogPageComp

		MessageCollectionView {
			anchors.fill: parent
			collectionId: "AgentLog"

			function getHeaders(){
				return {"clientid": agentEditorContainer.documentId}
			}
		}
	}

	// Interactive shell on the agent host. The document id is the agent id every
	// agent-scoped request in this editor is routed with (same value the Services and
	// Log pages put into their "clientid" header).
	Component {
		id: terminalPageComp

		TerminalView {
			anchors.fill: parent
			agentId: agentEditorContainer.documentId
		}
	}
}
