import QtQuick 2.15
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtgui 1.0

/**
	Status filter for AgentCollectionView. Structured like ImtCore's
	MessageCollectionFilterDecorator, but "status" is computed live server-side (connection +
	enrollment state, never stored - see CAgentCollectionControllerComp::ComputeAgentStatus), so
	it can't go through the generic ComplexFilter/FieldFilter/GroupFilter machinery
	MessageCollectionFilterDecorator uses - this emits bucketSelected(bucket) instead (a plain
	"status" GQL input param the server filters on itself), and only one bucket applies at a time.

	statusCounts/bucketSelected are NOT read off baseElement (unlike MessageCollectionFilterDecorator's
	complexFilter default) - when this is used as filterMenu.decorator, baseElement is the
	FilterMenu control itself (it re-exposes "complexFilter" as its own pass-through property,
	which is why that default works there), not the RemoteCollectionView, and FilterMenu has no
	concept of "statusCounts"/status filtering. The instantiating Component
	(AgentCollectionViewBase.qml) binds these explicitly instead, the same way
	MessageCollectionView.qml explicitly binds complexFilter rather than relying on baseElement.
*/
// No FilterPanelDecorator/advanced-filter popup here (unlike MessageCollectionFilterDecorator):
// the generic ComplexFilter machinery it drives is inert for the Agents collection regardless
// of field (CObjectCollectionBase::CreateObjectCollectionIterator ignores filter/offset/count
// entirely for this in-memory collection) - offering it would just be a control that silently
// does nothing. "status" filtering is handled directly below instead.
DecoratorBase {
	id: mainItem

	width: baseElement ? baseElement.width : 0
	height: Style.controlHeightL

	property alias segmentedButton: segmentedButton_
	property var statusCounts: null
	property int filterRightMargin: 0

	signal bucketSelected(string bucket)

	Component.onCompleted: {
		checkWidth();
	}

	onWidthChanged: {
		checkWidth();
	}

	onFilterRightMarginChanged: {
		checkWidth();
	}

	function checkWidth(){
		if (width - filterRightMargin <= segmentedButton.width + 2 * segmentedButton.spacing){
			segmentedButton.visible = false;
		}
		else{
			segmentedButton.visible = true;
		}
	}

	function countFor(bucket){
		if (!statusCounts){
			return 0;
		}
		let key = bucket === "" ? "all" : (bucket.charAt(0).toLowerCase() + bucket.slice(1));
		return statusCounts[key] !== undefined ? statusCounts[key] : 0;
	}

	function labelFor(bucket, text){
		return text + " (" + countFor(bucket) + ")";
	}

	function setFilter(bucket){
		allFilter.checked = (bucket === "");
		pendingFilter.checked = (bucket === "Pending");
		approvedFilter.checked = (bucket === "Approved");
		suspendedFilter.checked = (bucket === "Suspended");
		rejectedFilter.checked = (bucket === "Rejected");
		revokedFilter.checked = (bucket === "Revoked");

		mainItem.bucketSelected(bucket);
	}

	// Uses Style first/middle/last segment decorators (selectedColor fill + textSelectedColor).
	SegmentedButton {
		id: segmentedButton_
		anchors.left: parent.left;
		anchors.leftMargin: Style.marginM
		anchors.verticalCenter: parent.verticalCenter;

		height: parent.height

		Button {
			id: allFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			checked: true
			text: mainItem.labelFor("", qsTr("All"))
			iconSource: "../../../../" + Style.getIconPath("Icons/Dashboard", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("")
		}

		Button {
			id: pendingFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			text: mainItem.labelFor("Pending", qsTr("Pending"))
			iconSource: "../../../../" + Style.getIconPath("Icons/History", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("Pending")
		}

		Button {
			id: approvedFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			text: mainItem.labelFor("Approved", qsTr("Approved"))
			iconSource: "../../../../" + Style.getIconPath("Icons/Add", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("Approved")
		}

		Button {
			id: suspendedFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			text: mainItem.labelFor("Suspended", qsTr("Suspended"))
			iconSource: "../../../../" + Style.getIconPath("Icons/Stop", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("Suspended")
		}

		Button {
			id: rejectedFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			text: mainItem.labelFor("Rejected", qsTr("Rejected"))
			iconSource: "../../../../" + Style.getIconPath("Icons/Close", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("Rejected")
		}

		Button {
			id: revokedFilter;

			anchors.verticalCenter: parent.verticalCenter;
			checkable: true
			text: mainItem.labelFor("Revoked", qsTr("Revoked"))
			iconSource: "../../../../" + Style.getIconPath("Icons/Delete", Icon.State.On, Icon.Mode.Normal);
			widthFromDecorator: true;
			heightFromDecorator: true;
			onClicked: mainItem.setFilter("Revoked")
		}
	}
}
