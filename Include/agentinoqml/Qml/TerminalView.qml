import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0

Rectangle {
	id: root;

	// Agent the terminal is attached to (forwarded as the "clientid" header).
	property alias agentId: controller.agentId;

	color: Style.backgroundColor;

	TerminalController {
		id: controller;

		onOutputReceived: {
			root.appendOutput(data);
		}

		onSessionOpened: {
			root.appendOutput("\n[session " + sessionId + " opened]\n");
		}

		onSessionClosed: {
			root.appendOutput("\n[session closed, exit code " + exitCode + "]\n");
		}

		onShellTypesReceived: {
			shellModel.clear();
			let count = items.getItemsCount ? items.getItemsCount() : 0;
			for (let i = 0; i < count; ++i){
				let item = items.getItem(i);
				let index = shellModel.insertNewItem();
				shellModel.setData("id", item.containsKey("type") ? item.getData("type") : "", index);
				shellModel.setData("name", item.containsKey("name") ? item.getData("name") : "", index);
			}
		}

		onErrorOccurred: {
			root.appendOutput("\n[error] " + message + "\n");
		}
	}

	function appendOutput(text){
		outputEdit.text += text;
		outputEdit.cursorPosition = outputEdit.length;
		outputFlickable.contentY = Math.max(0, outputEdit.height - outputFlickable.height);
	}

	Component.onCompleted: {
		controller.listShellTypes();
	}

	Column {
		anchors.fill: parent;
		anchors.margins: Style.marginM;
		spacing: Style.marginS;

		Row {
			id: toolbar;
			width: parent.width;
			height: Style.itemSizeM;
			spacing: Style.marginS;

			ComboBox {
				id: shellSelector;
				height: Style.itemSizeM * 0.75;
				width: Style.itemSizeL;
				enabled: !controller.running;

				model: TreeItemModel {
					id: shellModel;
				}
			}

			Button {
				id: openButton;
				height: Style.itemSizeM * 0.75;
				width: 100;
				text: qsTr("Open");
				enabled: !controller.running && shellModel.getItemsCount() > 0;

				onClicked: {
					outputEdit.text = "";
					let shellType = shellModel.getData("id", shellSelector.currentIndex);
					controller.openSession(shellType);
				}
			}

			Button {
				id: closeButton;
				height: Style.itemSizeM * 0.75;
				width: 100;
				text: qsTr("Close");
				enabled: controller.running;

				onClicked: {
					controller.closeSession();
				}
			}

			Button {
				id: clearButton;
				height: Style.itemSizeM * 0.75;
				width: 100;
				text: qsTr("Clear");

				onClicked: {
					outputEdit.text = "";
				}
			}
		}

		Rectangle {
			id: outputContainer;
			width: parent.width;
			height: parent.height - toolbar.height - inputRow.height - parent.spacing * 2;
			color: Style.imagingToolsGradient0;
			border.color: Style.borderColor;
			border.width: 1;

			Flickable {
				id: outputFlickable;
				anchors.fill: parent;
				anchors.margins: Style.marginXS;
				clip: true;
				contentWidth: outputEdit.width;
				contentHeight: outputEdit.height;

				TextEdit {
					id: outputEdit;
					width: outputFlickable.width;
					readOnly: true;
					selectByMouse: true;
					wrapMode: TextEdit.WrapAnywhere;
					textFormat: TextEdit.PlainText;
					font.family: "Courier New";
					font.pixelSize: Style.fontSizeM;
					color: Style.textColor;
				}
			}
		}

		Row {
			id: inputRow;
			width: parent.width;
			height: Style.itemSizeM * 0.75;
			spacing: Style.marginS;

			CustomTextField {
				id: inputField;
				width: parent.width - sendButton.width - parent.spacing;
				height: parent.height;
				enabled: controller.running;
				placeHolderText: qsTr("Type a command and press Enter");

				function submit(){
					if (text.length > 0){
						controller.sendInput(text + "\n");
						root.appendOutput(text + "\n");
						text = "";
					}
				}

				Keys.onReturnPressed: inputField.submit();
				Keys.onEnterPressed: inputField.submit();
			}

			Button {
				id: sendButton;
				width: 100;
				height: parent.height;
				text: qsTr("Send");
				enabled: controller.running;

				onClicked: inputField.submit();
			}
		}
	}
}
