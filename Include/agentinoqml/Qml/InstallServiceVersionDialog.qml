import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0
import agentinoAgentsSdl 1.0

// "Install" command of the versions page of a service (ServiceVersionsView.qml): the approved agents
// to install the selected version on, chosen with check boxes. "Select all" checks or unchecks every
// agent and is partially checked while only some are. Installation is not implemented yet: the
// "Install" button only closes the dialog and reports the choice through installConfirmed().
Dialog {
	id: installDialog

	width: Style.sizeHintM
	backgroundColor: Style.baseColor

	title: installDialog.versionName !== ""
		   ? qsTr("Install version %1").arg(installDialog.versionName)
		   : qsTr("Install version")

	// IDs of the service versions to install
	property var versionIds: []
	// Shown in the title, e.g. "1.0.0.513"
	property string versionName: ""

	// Items: {id, name, checked}
	property ListModel agentsModel: ListModel {}
	property int checkedCount: 0
	property bool loading: false

	// Set while the "Select all" check box is updated from the list, so the update is not applied back.
	property bool updatingSelectAll: false

	signal installConfirmed(var versionIds, var agentIds)

	Component.onCompleted: {
		addButton(Enums.ok, qsTr("Install"), false)
		addButton(Enums.cancel, qsTr("Cancel"), true)

		installDialog.loadAgents()
	}

	onLocalizationChanged: {
		setButtonName(Enums.ok, qsTr("Install"))
		setButtonName(Enums.cancel, qsTr("Cancel"))
	}

	onFinished: {
		if (buttonId === Enums.ok){
			installDialog.installConfirmed(installDialog.versionIds, installDialog.getCheckedAgentIds())
		}
	}

	function loadAgents(){
		installDialog.loading = true
		agentsRequestSender.inputParams = Gql.GqlObject("input")
		agentsRequestSender.addInputParam("status", "Approved")
		agentsRequestSender.send()
	}

	function setAgents(items){
		installDialog.agentsModel.clear()

		let agents = []
		let count = items && items.count !== undefined ? items.count : 0
		for (let i = 0; i < count; ++i){
			let item = items.get(i).item
			if (item && item.m_id){
				agents.push({"id": String(item.m_id), "name": item.m_name ? String(item.m_name) : String(item.m_id), "checked": false})
			}
		}

		agents.sort(function(a, b){ return a.name.localeCompare(b.name) })
		for (let j = 0; j < agents.length; ++j){
			installDialog.agentsModel.append(agents[j])
		}

		installDialog.updateCheckedState()
	}

	function getCheckedAgentIds(){
		let result = []
		for (let i = 0; i < installDialog.agentsModel.count; ++i){
			if (installDialog.agentsModel.get(i).checked){
				result.push(installDialog.agentsModel.get(i).id)
			}
		}

		return result
	}

	function setAgentChecked(index, checked){
		installDialog.agentsModel.setProperty(index, "checked", checked)
		installDialog.updateCheckedState()
	}

	function setAllChecked(checked){
		for (let i = 0; i < installDialog.agentsModel.count; ++i){
			installDialog.agentsModel.setProperty(i, "checked", checked)
		}

		installDialog.updateCheckedState()
	}

	function updateCheckedState(){
		let count = 0
		for (let i = 0; i < installDialog.agentsModel.count; ++i){
			if (installDialog.agentsModel.get(i).checked){
				count++
			}
		}

		installDialog.checkedCount = count
		installDialog.setButtonEnabled(Enums.ok, count > 0)

		if (installDialog.bodyItem){
			installDialog.bodyItem.updateSelectAll()
		}
	}

	GqlSdlRequestSender {
		id: agentsRequestSender

		// Wire name (same as AgentinoAgentsSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "AgentsList"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")

			let itemsFields = Gql.GqlObject("items")
			itemsFields.InsertField("id")
			itemsFields.InsertField("name")
			payloadFields.InsertFieldObject(itemsFields)

			return payloadFields
		}

		sdlObjectComp: Component {
			AgentListPayload {
				onFinished: {
					installDialog.loading = false
					installDialog.setAgents(hasItems() ? m_items : null)
				}
			}
		}

		onFinished: {
			if (status < 0){
				installDialog.loading = false
			}
		}
	}

	contentComp: Component {
		Item {
			id: dialogBody

			width: installDialog.width
			height: Style.sizeHintXS + selectAllRow.height + Style.marginL

			function updateSelectAll(){
				let total = installDialog.agentsModel.count
				let state = installDialog.checkedCount === 0 ? Qt.Unchecked
						  : installDialog.checkedCount === total ? Qt.Checked : Qt.PartiallyChecked

				installDialog.updatingSelectAll = true
				selectAllCheckBox.checkState = state
				installDialog.updatingSelectAll = false
			}

			Component.onCompleted: {
				dialogBody.updateSelectAll()
			}

			// "Select all" with the number of the checked agents on the right.
			Item {
				id: selectAllRow

				anchors.top: parent.top
				anchors.topMargin: Style.marginM
				anchors.left: parent.left
				anchors.leftMargin: Style.marginL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginL

				height: Style.controlHeightM

				CheckBox {
					id: selectAllCheckBox

					objectName: "InstallSelectAllCheckBox"

					anchors.left: parent.left
					anchors.verticalCenter: parent.verticalCenter

					text: qsTr("Select all")
					isActive: installDialog.agentsModel.count > 0

					onCheckStateChanged: {
						if (installDialog.updatingSelectAll){
							return
						}

						// A click on the partially checked box checks it (CheckBox.nextCheckState).
						installDialog.setAllChecked(selectAllCheckBox.checkState === Qt.Checked)
					}
				}

				BaseText {
					anchors.right: parent.right
					anchors.verticalCenter: parent.verticalCenter

					visible: installDialog.agentsModel.count > 0
					text: qsTr("%1 of %2 selected").arg(installDialog.checkedCount).arg(installDialog.agentsModel.count)
					color: Style.subtitleColor
					font.pixelSize: Style.fontSizeS
				}
			}

			Rectangle {
				id: listFrame

				anchors.top: selectAllRow.bottom
				anchors.topMargin: Style.marginS
				anchors.left: parent.left
				anchors.leftMargin: Style.marginL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginL
				anchors.bottom: parent.bottom

				color: "transparent"
				radius: Style.radiusM
				border.width: 1
				border.color: Style.borderColor

				ListView {
					id: agentsList

					objectName: "InstallAgentsList"

					anchors.fill: parent
					anchors.margins: Style.marginXS
					anchors.rightMargin: agentsScrollbar.visible ? agentsScrollbar.width + Style.marginS : Style.marginXS

					clip: true
					boundsBehavior: Flickable.StopAtBounds
					model: installDialog.agentsModel

					delegate: Item {
						id: agentRow

						width: agentsList.width
						height: Style.controlHeightM

						// A click on the check box assigns its checkState, which would break a binding to the model;
						// the state is set from the model here instead, so "Select all" keeps updating the row.
						property bool rowChecked: model.checked

						onRowCheckedChanged: {
							agentCheckBox.checkState = agentRow.rowChecked ? Qt.Checked : Qt.Unchecked
						}

						Component.onCompleted: {
							agentCheckBox.checkState = agentRow.rowChecked ? Qt.Checked : Qt.Unchecked
						}

						Rectangle {
							anchors.fill: parent

							radius: Style.radiusS
							color: agentMouseArea.containsMouse ? Style.hover : "transparent"
						}

						CheckBox {
							id: agentCheckBox

							objectName: "InstallAgentCheckBox"

							anchors.left: parent.left
							anchors.leftMargin: Style.marginS
							anchors.verticalCenter: parent.verticalCenter

							text: model.name

							onCheckStateChanged: {
								let checked = agentCheckBox.checkState === Qt.Checked
								if (checked !== agentRow.rowChecked){
									installDialog.setAgentChecked(model.index, checked)
								}
							}
						}

						// The whole row toggles the agent, not only the check box with its text.
						MouseArea {
							id: agentMouseArea

							anchors.fill: parent
							z: -1

							hoverEnabled: true
							cursorShape: Qt.PointingHandCursor

							onClicked: {
								installDialog.setAgentChecked(model.index, !model.checked)
							}
						}
					}
				}

				// Shown and draggable whenever the list does not fit.
				CustomScrollbar {
					id: agentsScrollbar

					anchors.right: parent.right
					anchors.rightMargin: Style.marginXS
					anchors.top: parent.top
					anchors.topMargin: Style.marginXS
					anchors.bottom: parent.bottom
					anchors.bottomMargin: Style.marginXS

					secondSize: Style.marginS
					radius: Style.radiusS
					targetItem: agentsList
				}

				BaseText {
					anchors.centerIn: parent

					visible: !installDialog.loading && installDialog.agentsModel.count === 0
					text: qsTr("No approved agents")
					color: Style.inactiveTextColor
					font.pixelSize: Style.fontSizeM
				}

				BaseText {
					anchors.centerIn: parent

					visible: installDialog.loading
					text: qsTr("Loading...")
					color: Style.inactiveTextColor
					font.pixelSize: Style.fontSizeM
				}
			}
		}
	}
}
