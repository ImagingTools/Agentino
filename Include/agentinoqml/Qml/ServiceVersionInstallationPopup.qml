import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcontrols 1.0

// Popup message (bottom right, PopupManager.addCustomMessage) of an installation of a service version
// started by "Install" of the versions page (ServiceVersionsView.qml): the agents the installation was
// started on, with a spinning Loading while it runs on the agent and StateOk when the agent has installed
// the version (Error and the reason when it has not). It cannot be closed while the installation runs;
// at the end it shows the result: ServiceVersionsView closes it when every agent has been updated, an
// installation that failed on some agents is closed by the close button.
Rectangle {
	id: installationPopup

	objectName: "ServiceVersionInstallationPopup"

	// Set by the popup container (PopupContainer.qml)
	property string messageId: ""
	property var popupContainer: null

	// The followed installation (installationProgressComp of ServiceVersionsView.qml): version, agents
	// (ListModel: agentId, agentName, state - "Running", "Succeeded" or "Failed", message), finished, resultText
	property var progress: null

	readonly property bool isFinished: installationPopup.progress ? installationPopup.progress.finished : false

	width: parent ? parent.width : Style.sizeHintS
	height: contentColumn.height + 2 * Style.marginM
	radius: Style.radiusM
	color: Style.baseColor
	border.width: 1
	border.color: Style.inputBorderColor
	clip: true

	Column {
		id: contentColumn

		anchors.left: parent.left
		anchors.leftMargin: Style.marginM
		anchors.right: parent.right
		anchors.rightMargin: Style.marginS
		anchors.top: parent.top
		anchors.topMargin: Style.marginM

		spacing: Style.marginS

		Item {
			id: titleRow

			width: contentColumn.width
			height: Math.max(titleText.height, closeButton.height)

			BaseText {
				id: titleText

				anchors.left: parent.left
				anchors.right: closeButton.left
				anchors.rightMargin: Style.marginS
				anchors.verticalCenter: parent.verticalCenter

				text: installationPopup.progress ? qsTr("Installation of the version %1").arg(installationPopup.progress.version) : ""
				wrapMode: Text.WordWrap
				color: Style.textColor
				font.family: Style.fontFamilyBold
				font.pixelSize: Style.fontSizeM
			}

			// Only an ended installation can be closed.
			Rectangle {
				id: closeButton

				anchors.right: parent.right
				anchors.verticalCenter: parent.verticalCenter

				width: Style.buttonWidthM
				height: Style.buttonWidthM
				radius: Style.buttonRadius
				visible: installationPopup.isFinished
				color: closeMouseArea.containsMouse ? Style.toolButtonHoverColor : "transparent"

				Image {
					anchors.centerIn: parent
					width: Style.iconSizeS
					height: width
					sourceSize.width: width
					sourceSize.height: height
					source: "qrc:/" + Style.getIconPath("Icons/Close", Icon.State.On, Icon.Mode.Normal)
				}

				MouseArea {
					id: closeMouseArea

					anchors.fill: parent
					hoverEnabled: true
					cursorShape: Qt.PointingHandCursor
					onClicked: {
						if (installationPopup.popupContainer){
							installationPopup.popupContainer.removeMessageById(installationPopup.messageId)
						}
					}
				}
			}
		}

		Repeater {
			id: agentsRepeater

			model: installationPopup.progress ? installationPopup.progress.agents : null

			delegate: Item {
				id: agentRow

				objectName: "InstallationAgentRow"

				width: contentColumn.width
				height: Math.max(Style.iconSizeS, agentTextColumn.height)

				Column {
					id: agentTextColumn

					anchors.left: parent.left
					anchors.right: stateArea.left
					anchors.rightMargin: Style.marginS
					anchors.verticalCenter: parent.verticalCenter

					spacing: Style.marginXS

					BaseText {
						width: parent.width
						text: model.agentName
						elide: Text.ElideRight
						color: Style.textColor
						font.pixelSize: Style.fontSizeM
					}

					BaseText {
						width: parent.width
						visible: model.state === "Failed" && model.message !== ""
						text: model.message
						wrapMode: Text.WordWrap
						maximumLineCount: 3
						elide: Text.ElideRight
						color: Style.subtitleColor
						font.pixelSize: Style.fontSizeS
					}
				}

				// Loading while the agent installs the version, then its result.
				Item {
					id: stateArea

					anchors.right: parent.right
					anchors.rightMargin: (Style.buttonWidthM - Style.iconSizeS) / 2
					anchors.verticalCenter: parent.verticalCenter

					width: Style.iconSizeS
					height: Style.iconSizeS

					Loading {
						anchors.fill: parent
						indicatorSize: Style.iconSizeS
						background.color: "transparent"
						visible: model.state === "Running"
					}

					Image {
						anchors.fill: parent
						sourceSize.width: width
						sourceSize.height: height
						fillMode: Image.PreserveAspectFit
						visible: model.state !== "Running"
						source: model.state === "Succeeded"
								? "qrc:/" + Style.getIconPath("Icons/StateOk", Icon.State.On, Icon.Mode.Normal)
								: model.state === "Failed"
								? "qrc:/" + Style.getIconPath("Icons/Error", Icon.State.On, Icon.Mode.Normal)
								: ""
					}
				}
			}
		}

		// The result of the ended installation.
		BaseText {
			id: resultText

			width: contentColumn.width
			visible: installationPopup.isFinished && text !== ""
			text: installationPopup.progress ? installationPopup.progress.resultText : ""
			wrapMode: Text.WordWrap
			color: Style.textColor
			font.family: Style.fontFamilyBold
			font.pixelSize: Style.fontSizeM
		}
	}
}
