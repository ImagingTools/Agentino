import QtQuick 2.0
import Acf 1.0
import imtgui 1.0
import imtcontrols 1.0
import imtauthgui 1.0

/**
	The token an output connection holds at the server it goes to. The server issues it for itself, so the operator
	signs in there as a superuser for every change; the agent keeps the value and passes it to the service at its start.
*/
ElementView {
	id: tokenView

	name: qsTr("Server Token")
	description: tokenView.getStatusText()

	property string serviceId: ""
	property string connectionId: ""

	// Creates the transport of this view; set by the editor
	property Component transportComp: null
	property var headersProvider: null

	property var transport: null
	property var token: null
	property bool restartRequired: false
	property bool busy: false

	// A stopped service reads the token at its next start, so only a running one needs a restart
	property bool serviceRunning: false

	onServiceRunningChanged: {
		if (!tokenView.serviceRunning){
			tokenView.restartRequired = false
		}
	}

	// What the sign-in at the server is for: "set" or "revoke"
	property string pendingAction: ""

	// The token is read once the row is shown for a linked connection
	Component.onCompleted: {
		tokenView.requestTokenIfShown()
	}

	onVisibleChanged: {
		tokenView.requestTokenIfShown()
	}

	onConnectionIdChanged: {
		tokenView.requestTokenIfShown()
	}

	onServiceIdChanged: {
		tokenView.requestTokenIfShown()
	}

	function requestTokenIfShown(){
		if (tokenView.transport || !tokenView.transportComp || !tokenView.visible || tokenView.serviceId === "" || tokenView.connectionId === ""){
			return
		}

		tokenView.transport = tokenView.transportComp.createObject(tokenView)
		tokenView.transport.headersProvider = tokenView.headersProvider
		tokenView.transport.requestToken(tokenView.serviceId, tokenView.connectionId)
	}

	Connections {
		target: tokenView.transport

		function onTokenReceived(token){
			tokenView.token = token
		}

		function onCatalogReceived(catalog){
			if (tokenView.pendingAction === "revoke"){
				tokenView.transport.revokeToken(tokenView.serviceId, tokenView.connectionId, catalog.sessionToken)

				return
			}

			tokenView.busy = false
			ModalDialogManager.openDialog(permissionsDialogComp, {"catalog": catalog})
		}

		function onTokenChanged(token, restartRequired){
			tokenView.busy = false
			tokenView.token = token
			tokenView.restartRequired = tokenView.serviceRunning && (tokenView.restartRequired || restartRequired)
		}

		function onTokenRevoked(){
			tokenView.busy = false
			tokenView.token = null
			// The running process keeps the revoked token until it is restarted
			tokenView.restartRequired = tokenView.serviceRunning
		}

		function onFailed(message){
			tokenView.busy = false
			PopupManager.addErrorMessage(message, true)
		}
	}

	function getStatusText(){
		if (tokenView.busy){
			return qsTr("Waiting for the server...")
		}

		if (!tokenView.token){
			let noTokenText = qsTr("No token: the server at the other end refuses this service")

			return tokenView.restartRequired ? noTokenText + ". " + qsTr("Restart the service to stop using the revoked token.") : noTokenText
		}

		let issuedAt = Qt.formatDateTime(new Date(tokenView.token.issuedAt), "yyyy-MM-dd HH:mm")
		let count = tokenView.token.scopes.length
		let text = (count === 1 ? qsTr("1 permission at %2, issued %3") : qsTr("%1 permissions at %2, issued %3").arg(count)).arg(tokenView.token.receiverProductId).arg(issuedAt)
		if (tokenView.restartRequired){
			text = text + ". " + qsTr("Restart the service to use the new token.")
		}

		return text
	}

	function startAction(action){
		tokenView.pendingAction = action
		ModalDialogManager.openDialog(signInDialogComp, {})
	}

	controlComp: Component {
		Row {
			spacing: Style.marginM

			Button {
				objectName: "ConnectionTokenSetButton"
				enabled: !tokenView.busy
				text: tokenView.token ? qsTr("Permissions...") : qsTr("Issue Token...")
				onClicked: {
					tokenView.startAction("set")
				}
			}

			Button {
				objectName: "ConnectionTokenRevokeButton"
				visible: tokenView.token !== null
				enabled: !tokenView.busy
				text: qsTr("Revoke")
				onClicked: {
					tokenView.startAction("revoke")
				}
			}
		}
	}

	Component {
		id: signInDialogComp

		Dialog {
			id: signInDialog
			title: tokenView.pendingAction === "revoke" ? qsTr("Revoke the token") : qsTr("Sign in to the server")
			width: 420
			height: 340
			backgroundColor: Style.baseColor

			property string login: ""
			property string password: ""

			function updateApplyButton(){
				signInDialog.setButtonEnabled(Enums.apply, signInDialog.login !== "" && signInDialog.password !== "")
			}

			onFinished: {
				if (buttonId !== Enums.apply)
					return

				tokenView.busy = true
				tokenView.transport.requestCatalog(tokenView.serviceId, tokenView.connectionId, signInDialog.login, signInDialog.password)
			}

			Component.onCompleted: {
				clearButtons()
				addButton(Enums.apply, tokenView.pendingAction === "revoke" ? qsTr("Revoke") : qsTr("Sign in"), false)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			contentComp: Component {
				Item {
					width: signInDialog.width
					height: signInContent.height + 2 * Style.marginL

					Column {
						id: signInContent
						anchors.top: parent.top
						anchors.topMargin: Style.marginL
						anchors.horizontalCenter: parent.horizontalCenter
						width: parent.width - 2 * Style.marginXL
						spacing: Style.marginM

						BaseText {
							width: parent.width
							wrapMode: Text.WordWrap
							text: qsTr("The server the connection goes to issues the token. Sign in there as a superuser; the sign-in is not kept.")
							color: Style.inactiveTextColor
						}

						CustomTextField {
							objectName: "ConnectionTokenLoginInput"
							width: parent.width
							height: Style.controlHeightM
							placeHolderText: qsTr("Login")
							onTextChanged: {
								signInDialog.login = text.trim()
								signInDialog.updateApplyButton()
							}
						}

						CustomTextField {
							objectName: "ConnectionTokenPasswordInput"
							width: parent.width
							height: Style.controlHeightM
							placeHolderText: qsTr("Password")
							echoMode: TextInput.Password
							onTextChanged: {
								signInDialog.password = text
								signInDialog.updateApplyButton()
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: permissionsDialogComp

		Dialog {
			id: permissionsDialog
			title: qsTr("Permissions of the token")
			width: 680
			height: 760
			backgroundColor: Style.baseColor

			// Set by openDialog after the dialog is created
			property var catalog: null

			// Set by the content when it is created
			property var permissionsView: null

			onCatalogChanged: {
				permissionsDialog.updatePermissions()
			}

			// The permissions every token holds are checked and locked
			function updatePermissions(){
				if (!permissionsDialog.permissionsView || !permissionsDialog.catalog){
					return
				}

				permissionsDialog.permissionsView.rebuildFromFlatArray(permissionsDialog.catalog.groups)
				permissionsDialog.permissionsView.applySelection(permissionsDialog.catalog.token ? permissionsDialog.catalog.token.scopes : [])

				let locked = {}
				let required = permissionsDialog.catalog.requiredPermissions
				for (let i = 0; i < required.length; ++i){
					locked[required[i]] = qsTr("Every token for this server holds it")
				}
				permissionsDialog.permissionsView.setLockedPermissions(locked)
			}

			onFinished: {
				if (buttonId !== Enums.apply || !permissionsDialog.permissionsView)
					return

				tokenView.busy = true
				tokenView.transport.setToken(tokenView.serviceId, tokenView.connectionId, permissionsDialog.catalog.sessionToken, permissionsDialog.permissionsView.getCheckedIds(false))
			}

			Component.onCompleted: {
				clearButtons()
				addButton(Enums.apply, qsTr("Apply"), true)
				addButton(Enums.cancel, qsTr("Cancel"), true)
			}

			contentComp: Component {
				Item {
					width: permissionsDialog.width
					height: permissionsDialog.height - 3 * Style.controlHeightM - 2 * Style.marginXL

					Component.onCompleted: {
						permissionsDialog.permissionsView = permissionsView
						permissionsDialog.updatePermissions()
					}

					BaseText {
						id: receiverText
						anchors.top: parent.top
						anchors.topMargin: Style.marginL
						anchors.left: parent.left
						anchors.leftMargin: Style.marginXL
						text: permissionsDialog.catalog ? qsTr("Server accepting the token: %1").arg(permissionsDialog.catalog.receiverProductId) : ""
					}

					Rectangle {
						anchors.top: receiverText.bottom
						anchors.topMargin: Style.marginM
						anchors.bottom: parent.bottom
						anchors.left: parent.left
						anchors.leftMargin: Style.marginXL
						anchors.right: parent.right
						anchors.rightMargin: Style.marginXL
						color: Style.baseColor
						border.width: 1
						border.color: Style.borderColor
						radius: Style.radiusM
						clip: true

						PermissionsTableView {
							id: permissionsView
							objectName: "ConnectionTokenPermissionsView"
							anchors.fill: parent
							anchors.leftMargin: Style.marginM
							anchors.rightMargin: Style.marginM
							anchors.topMargin: Style.marginXS
							anchors.bottomMargin: Style.marginS
							controlPanelTopMargin: Style.marginS
							treeTopMargin: Style.marginXS
							treeBottomMargin: Style.marginS
						}
					}
				}
			}
		}
	}
}
