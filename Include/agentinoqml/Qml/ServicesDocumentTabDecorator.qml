import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0

/**
	AgentEditor Services MultiDoc tab chrome only.

	Flat document-title row: no dark selected block, no orange strip, no chips.
	Active tab = titleColor + blue bar across that tab's full width; inactive =
	muted subtitleColor. Subtle divider between tabs (hidden next to the selected
	tab). Close glyph only on non-pinned tabs when selected/hovered; slot width is fixed.
*/
DecoratorBase {
	id: root

	width: contentRow.width + 2 * Style.marginM
	height: baseElement ? baseElement.height : Style.headerHeight

	property bool isSelected: root.baseElement ? root.baseElement.selected : false
	property bool isPinned: root.baseElement ? root.baseElement.pinned : false
	property bool isHovered: root.baseElement && root.baseElement.mouseArea
			? root.baseElement.mouseArea.containsMouse
			: false
	// Slot is always reserved for closable tabs so hover does not resize the tab.
	property bool canClose: root.baseElement
			&& !root.baseElement.pinned
			&& root.baseElement.isCloseEnable
	property bool showClose: root.canClose && (root.isSelected || root.isHovered)
	property int closeSlotSize: 12
	// Divider on the right: not after the last tab, not beside the selected tab
	// (neither when this tab is selected nor when the next one is).
	property bool showDivider: root.baseElement
			&& root.baseElement.listView
			&& root.baseElement.index < root.baseElement.listView.count - 1
			&& !root.baseElement.selected
			&& (root.baseElement.index + 1) !== root.baseElement.selectedIndex

	Connections {
		target: root.baseElement

		function onStartContentLoading(){
			loading.start()
		}

		function onStopContentLoading(){
			loading.stop()
		}
	}

	// Light gray hover wash (not on the selected tab — underline already marks it).
	Rectangle {
		anchors.fill: parent
		anchors.topMargin: Style.marginXS
		anchors.bottomMargin: Style.marginXS
		anchors.leftMargin: 2
		anchors.rightMargin: 2
		radius: Style.marginS
		color: Style.alternateBaseColor
		visible: root.isHovered && !root.isSelected
	}

	// Blue accent across the full width of this tab only (not the whole panel).
	Rectangle {
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		height: root.isSelected ? 2 : 0
		color: Style.textSelectedColor
	}

	// Subtle separator between tabs (hidden next to the selected tab).
	Rectangle {
		anchors.right: parent.right
		anchors.verticalCenter: parent.verticalCenter
		width: 1
		height: parent.height * 0.4
		visible: root.showDivider
		color: Style.borderColor
		opacity: 0.55
	}

	Row {
		id: contentRow

		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		height: parent.height
		spacing: Style.marginS
		visible: !loading.visible

		Item {
			id: iconItem

			anchors.verticalCenter: parent.verticalCenter
			width: tabIcon.status === Image.Ready ? tabIcon.width : 0
			height: parent.height
			visible: width > 0

			Image {
				id: tabIcon

				anchors.centerIn: parent
				width: Style.iconSizeS
				height: width
				source: root.baseElement && root.baseElement.icon !== ""
						? ("qrc:/" + Style.getIconPath(
							   root.baseElement.icon,
							   Icon.State.On,
							   root.isSelected ? Icon.Mode.Selected : Icon.Mode.Normal))
						: ""
				sourceSize.width: width
				sourceSize.height: height
				fillMode: Image.PreserveAspectFit
				opacity: root.isSelected || root.isHovered ? 1 : 0.75
			}
		}

		Item {
			id: labelItem

			anchors.verticalCenter: parent.verticalCenter
			height: parent.height
			width: root.baseElement && textHelper.width < root.baseElement.minWidth
					? root.baseElement.minWidth
					: (root.baseElement && textHelper.width > root.baseElement.maxWidth
						? root.baseElement.maxWidth
						: textHelper.width)

			Text {
				id: textHelper
				visible: false
				text: root.baseElement ? root.baseElement.text : ""
				font.family: Style.fontFamily
				font.pixelSize: Style.fontSizeM
				font.bold: root.isSelected || root.isPinned
			}

			Text {
				id: label

				anchors.verticalCenter: parent.verticalCenter
				anchors.horizontalCenter: parent.horizontalCenter
				width: parent.width
				text: root.baseElement ? root.baseElement.text : ""
				font.family: Style.fontFamily
				font.pixelSize: Style.fontSizeM
				font.bold: root.isSelected || root.isPinned
				color: root.isSelected
						? Style.titleColor
						: (root.isHovered ? Style.textColor : Style.subtitleColor)
				elide: Text.ElideRight
				horizontalAlignment: Text.AlignHCenter
			}
		}

		// Fixed-width close slot: width never collapses, only the glyph fades in/out.
		Item {
			id: closeSlot

			anchors.verticalCenter: parent.verticalCenter
			width: root.canClose ? root.closeSlotSize : 0
			height: root.closeSlotSize
			visible: root.canClose

			ToolButton {
				id: closeButton

				objectName: "CloseButton"
				anchors.centerIn: parent
				width: root.closeSlotSize
				height: root.closeSlotSize
				opacity: root.showClose ? 1 : 0
				enabled: root.showClose
				iconSource: "qrc:/" + Style.getIconPath("Icons/Close", Icon.State.On, Icon.Mode.Normal)
				decorator: Component {
					ToolButtonDecorator {
						color: "transparent"
						icon.width: 10
					}
				}

				onClicked: {
					if (root.baseElement)
						root.baseElement.closeSignal()
				}
			}
		}
	}

	// Spinner only over the label (not the whole tab / close slot), so a stuck
	// waitName load cannot block closing the tab.
	Loading {
		id: loading
		anchors.centerIn: labelItem
		width: Style.controlHeightS
		height: Style.controlHeightS
		indicatorSize: Style.iconSizeS
		background.color: "transparent"
		visible: false
		z: 5
	}
}
