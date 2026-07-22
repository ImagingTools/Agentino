import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0

/**
	Shared modern table chrome for Agentino Agents / Services collections.

	CollectionViewBase defaults still draw a dense vertical grid and a compact
	row height that reads as legacy. This helper flattens the grid, softens
	zebra striping, and opens up row/header spacing — only for the tables that
	call apply(), not globally for every Table in ImtCore.
*/
QtObject {
	id: root

	function apply(table){
		if (!table)
			return

		// Surfaces match AgentEditor / Services MultiDoc (baseColor).
		table.backgroundElementsColor = Style.baseColor
		table.backgroundHeadersColor = Style.baseColor

		// Soft zebra + hover (row chrome lives in TableRowDelegateBase).
		table.enableAlternating = true
		table.alternatingColor = Style.alternateBaseColor
		table.alternatingOpacity = 0.4
		table.hoverEnabled = true

		// Roomier rows / header than Style.tableRowHeight (35).
		table.itemHeight = 42
		table.headerHeight = 40

		// Flat list look: no vertical or cell grid lines.
		table.verticalBorderSize = 0
		table.horizontalBorderSize = 0
		table.borderColorVertical = "transparent"
		table.borderColorHorizontal = "transparent"
		table.verticalBorderHeight = 0
		table.visibleLeftBorderFirst = false
		table.visibleRightBorderLast = false
		table.visibleTopBorderFirst = false
		table.visibleBottomBorderLast = false

		table.verticalBorderSize_deleg = 0
		table.horizontalBorderSize_deleg = 0
		table.borderColorVertical_deleg = "transparent"
		table.borderColorHorizontal_deleg = "transparent"
		table.visibleLeftBorderFirst_deleg = false
		table.visibleRightBorderLast_deleg = false
		table.visibleTopBorderFirst_deleg = false
		table.visibleBottomBorderLast_deleg = false
		table.isRightBorder = false
		table.isRightBorder_deleg = false

		// Comfortable horizontal padding for text/icons.
		table.textMarginHor = Style.marginL
		table.textMarginHor_deleg = Style.marginL
		table.textMarginVer = Style.marginM
		table.textMarginVer_deleg = Style.marginM

		// Thin line under the header strip (TableBase bottomLine).
		table.separatorVisible = true
	}
}
