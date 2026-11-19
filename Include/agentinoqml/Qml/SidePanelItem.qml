import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0

// Item of a side panel of a page (the operating systems of UpdatePage, the views of TopologyPage),
// styled as the page list of MultiPageView - e.g. General / Services / Agent Log / Terminal of AgentEditor.
Item {
	id: sidePanelItem

	property string text: ""
	// Icon ID, e.g. "Icons/System"; empty shows no icon
	property string icon: ""
	// Short text on the right, e.g. a count; empty shows none
	property string badge: ""
	property bool selected: false

	readonly property bool highlighted: itemMouseArea.containsMouse
	property alias mouseArea: itemMouseArea

	signal clicked()

	height: Style.controlHeightM

	Rectangle {
		anchors.fill: parent

		radius: Style.marginS
		color: sidePanelItem.selected ? Style.selectedColor
			: sidePanelItem.highlighted ? Style.hover : "transparent"
	}

	Row {
		anchors.fill: parent
		anchors.leftMargin: Style.marginM
		anchors.rightMargin: Style.marginM

		spacing: Style.marginS

		Image {
			id: itemIcon

			anchors.verticalCenter: parent.verticalCenter

			visible: sidePanelItem.icon !== ""
			width: Style.iconSizeS
			height: Style.iconSizeS
			sourceSize.width: width
			sourceSize.height: height
			source: sidePanelItem.icon !== ""
				? "qrc:/" + Style.getIconPath(sidePanelItem.icon, Icon.State.On,
											sidePanelItem.selected ? Icon.Mode.Selected : Icon.Mode.Normal)
				: ""
		}

		Text {
			anchors.verticalCenter: parent.verticalCenter

			width: parent.width - (itemIcon.visible ? itemIcon.width + parent.spacing : 0)
				   - (itemBadge.visible ? itemBadge.width + parent.spacing : 0)

			text: sidePanelItem.text
			font.family: Style.fontFamily
			font.pixelSize: Style.fontSizeM
			color: Style.textColor
			elide: Text.ElideRight
		}

		Text {
			id: itemBadge

			anchors.verticalCenter: parent.verticalCenter

			visible: sidePanelItem.badge !== ""
			text: sidePanelItem.badge
			font.family: Style.fontFamily
			font.pixelSize: Style.fontSizeS
			color: sidePanelItem.selected ? Style.textSelectedColor : Style.subtitleColor
		}
	}

	MouseArea {
		id: itemMouseArea

		objectName: "MouseArea"

		anchors.fill: parent

		hoverEnabled: true
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			sidePanelItem.clicked()
		}
	}
}
