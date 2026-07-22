import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0

/**
	Terminal multi-tab strip chrome (AgentEditor Terminal page only).

	Matches ServicesDocumentTabDecorator language: flat baseColor strip, title/subtitle
	text, blue underline on the active tab, soft hover wash, fixed close slot.
	Slightly tighter than document tabs so many shell sessions fit when the page
	is narrow (width is driven by TabDelegate minWidth/maxWidth).
*/
DecoratorBase {
	id: root

	width: contentRow.width + 2 * Style.marginM
	height: baseElement ? baseElement.height : Style.controlHeightL

	property bool isSelected: root.baseElement ? root.baseElement.selected : false
	property bool isPinned: root.baseElement ? root.baseElement.pinned : false
	property bool isHovered: root.baseElement && root.baseElement.mouseArea
			? root.baseElement.mouseArea.containsMouse
			: false
	// Slot always reserved for closable tabs so hover does not resize the tab.
	property bool canClose: root.baseElement
			&& !root.baseElement.pinned
			&& root.baseElement.isCloseEnable
	property bool showClose: root.canClose && (root.isSelected || root.isHovered)
	property int closeSlotSize: 14
	// Extra gap between the title and the close glyph (wider, less cramped tabs).
	property int textToCloseGap: Style.marginM
	// Divider on the right: not after the last tab, not beside the selected tab.
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

	// Soft hover wash (not on the selected tab — underline already marks it).
	Rectangle {
		anchors.fill: parent
		anchors.topMargin: Style.marginXS
		anchors.bottomMargin: Style.marginXS
		anchors.leftMargin: 1
		anchors.rightMargin: 1
		radius: Style.radiusM
		color: Style.alternateBaseColor
		visible: root.isHovered && !root.isSelected
	}

	// Blue accent across the full width of this tab only.
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
		spacing: Style.spacingS
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
				opacity: root.isSelected || root.isHovered ? 1 : 0.7
			}
		}

		Item {
			id: labelItem

			anchors.verticalCenter: parent.verticalCenter
			height: parent.height
			// Prefer baseElement min/max so the host can compress tabs on narrow pages.
			width: {
				if (!root.baseElement){
					return textHelper.width
				}
				let w = textHelper.width
				if (w < root.baseElement.minWidth){
					return root.baseElement.minWidth
				}
				if (w > root.baseElement.maxWidth){
					return root.baseElement.maxWidth
				}
				return w
			}

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
				anchors.left: parent.left
				width: parent.width
				text: root.baseElement ? root.baseElement.text : ""
				font.family: Style.fontFamily
				font.pixelSize: Style.fontSizeM
				font.bold: root.isSelected || root.isPinned
				color: root.isSelected
						? Style.titleColor
						: (root.isHovered ? Style.textColor : Style.subtitleColor)
				elide: Text.ElideRight
				horizontalAlignment: Text.AlignLeft
			}
		}

		// Breathing room between title and close — keeps tabs from looking cramped.
		Item {
			anchors.verticalCenter: parent.verticalCenter
			width: root.canClose ? root.textToCloseGap : 0
			height: 1
			visible: root.canClose
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
					if (root.baseElement){
						root.baseElement.closeSignal()
					}
				}
			}
		}
	}

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
