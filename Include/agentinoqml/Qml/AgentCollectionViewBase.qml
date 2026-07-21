import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import agentinoAgentsSdl 1.0
import agentinoEnrollmentSdl 1.0

RemoteCollectionView {
	// Not named "root": PopupView has property root (= ModalDialogManager) and would
	// shadow any outer id "root" from the dialog creation context.
	id: agentsView;

	visibleMetaInfo: false;

	collectionId: "Agents";

	// "status" is computed live server-side (connection state + enrollment state), never
	// stored, so the generic ComplexFilter/collectionFilter machinery this view's
	// filterMenu/FilterPanelDecorator would normally drive can't see it at all - CAgentCollectionControllerComp::ListObjects
	// filters directly on a plain "status" input param instead. This overrides the default
	// dataControllerComp (see RemoteCollectionView.qml) verbatim, just adding that one param.
	property string statusFilter: ""
	property var statusCounts: ({"all": 0, "pending": 0, "approved": 0, "suspended": 0, "rejected": 0, "revoked": 0})

	function setStatusFilter(bucket){
		agentsView.statusFilter = bucket;
		agentsView.doUpdateGui();
	}

	function refreshStatusCounts(){
		statusCountsRequestSender.send();
	}

	dataControllerComp: Component {
		CollectionRepresentation {
			collectionId: agentsView.collectionId;
			gqlGetListCommandId: agentsView.gqlGetListCommandId
			requestedFields: agentsView.requestedFields

			additionalFieldIds: agentsView.additionalFieldIds;

			onRemoved: {
				agentsView.removed()
			}

			function getHeaders(){
				return agentsView.getHeaders();
			}

			function addAdditionalInputParams(inputParams){
				if (agentsView.statusFilter !== ""){
					inputParams.InsertField("status", agentsView.statusFilter);
				}
			}
		}
	}

	// Independent of the main list fetch above (own round trip to the same AgentsList query,
	// requesting only "counts") so the filter buttons can show "Pending (2)" etc. without
	// depending on RemoteCollectionView's item/notification-only response handling.
	GqlSdlRequestSender {
		id: statusCountsRequestSender
		gqlCommandId: AgentinoAgentsSdlCommandIds.s_agentsList

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload");

			// The server always builds the full "items" array regardless of selection (its
			// ListObjects override doesn't gate the array itself on request fields, only the
			// per-item field population does) - but the client's generated fromObject() needs
			// a well-formed item (at least one recognized field) per array entry to resolve
			// each AgentItem via createElement(), or it throws trying to parse a bare {}. "id"
			// alone is enough and keeps this request cheap.
			let itemsFields = Gql.GqlObject("items");
			itemsFields.InsertField("id");
			payloadFields.InsertFieldObject(itemsFields);

			let countsFields = Gql.GqlObject("counts");
			countsFields.InsertField("all");
			countsFields.InsertField("pending");
			countsFields.InsertField("approved");
			countsFields.InsertField("suspended");
			countsFields.InsertField("rejected");
			countsFields.InsertField("revoked");
			payloadFields.InsertFieldObject(countsFields);
			return payloadFields;
		}

		sdlObjectComp: Component {
			AgentListPayload {
				onFinished: {
					console.log("AgentListPayload onFinished")
					if (hasCounts()){
						agentsView.statusCounts = {
							"all": m_counts.m_all,
							"pending": m_counts.m_pending,
							"approved": m_counts.m_approved,
							"suspended": m_counts.m_suspended,
							"rejected": m_counts.m_rejected,
							"revoked": m_counts.m_revoked
						};
						console.log("agentsView.statusCounts", agentsView.statusCounts)
					}
				}
			}
		}
	}

	Component {
		id: agentStatusFilterComp;

		AgentStatusFilterDecorator {
			// Explicit, not baseElement-derived: when used as filterMenu.decorator, baseElement
			// is the FilterMenu control itself, not agentsView (see AgentStatusFilterDecorator.qml).
			statusCounts: agentsView.statusCounts
			onBucketSelected: agentsView.setStatusFilter(bucket)
		}
	}

	// Single page for every agent regardless of enrollment status (Pending/Approved/
	// Suspended/Rejected/Revoked all get a row here - see
	// CAgentCollectionControllerComp::InsertObject). Approve/Reject/Suspend/Resume/Revoke/
	// Reset live in this page's toolbar (enabled per selected row's status, see
	// AgentCollectionViewCommandsDelegate.qml) instead of a separate Enrollment page.
	property var enrollmentViewModel: GqlBasedEnrollmentViewModel {
		onDecisionFinished: {
			agentsView.doUpdateGui();
			agentsView.refreshStatusCounts();
		}
	}

	// Single source of truth for status -> color, shared with the status badge below.
	// Picked from StyleBase.qml's actual palette rather than the nominally-named
	// positiveAccentColor/greenColor (neon cyan/green - clash with everything else here).
	function colorForStatus(status){
		if (status === "Approved" || status === "Connected"){
			return Style.titleColor
		}
		if (status === "Rejected" || status === "Revoked"){
			return Style.negativeAccentColor
		}
		if (status === "Pending"){
			return Style.imaginToolsAccentColor
		}
		if (status === "Suspended" || status === "Disconnected"){
			return Style.subtitleColor
		}
		return Style.inactiveTextColor
	}

	commandsDelegateComp: Component {AgentCollectionViewCommandsDelegate {
			collectionView: agentsView
			enrollmentViewModel: agentsView.enrollmentViewModel

			onCommandActivated: {
				let enrollmentCommandIds = ["Approve", "Reject", "Suspend", "Resume", "Revoke", "Reset"];
				if (enrollmentCommandIds.indexOf(commandId) < 0){
					return;
				}

				let indexes = agentsView.table.getSelectedIndexes();
				if (indexes.length === 0){
					return;
				}
				let index = indexes[0];
				let agentId = agentsView.table.elements.getData("id", index);
				let name = agentsView.table.elements.getData("computerName", index);

				switch (commandId){
				case "Approve":
					agentsView.enrollmentViewModel.requestApprove(agentId, name, "");
					break;
				case "Reject":
					agentsView.enrollmentViewModel.requestReject(agentId, "");
					break;
				case "Suspend":
					agentsView.enrollmentViewModel.requestSuspend(agentId, true);
					break;
				case "Resume":
					agentsView.enrollmentViewModel.requestSuspend(agentId, false);
					break;
				case "Revoke":
					agentsView.enrollmentViewModel.requestRevoke(agentId, "");
					break;
				case "Reset":
					agentsView.enrollmentViewModel.requestResetRejected(agentId);
					break;
				}
			}
		}
	}

	Component.onCompleted: {
		let documentManagerPtr = MainDocumentService.getDocumentService(agentsView.collectionId)
		if (documentManagerPtr && agentsView.commandsDelegate){
			agentsView.commandsDelegate.documentManager = documentManagerPtr
		}

		agentsView.filterMenu.decorator = agentStatusFilterComp;
		agentsView.refreshStatusCounts();
	}

	onHeadersChanged: {
		if (agentsView.table.headers.getItemsCount() > 0){
			let orderIndex = agentsView.table.getHeaderIndex("status");
			if (orderIndex >= 0){
				agentsView.table.setColumnContentComponent(orderIndex, stateColumnContentComp);
			}
		}
	}

	// Double-click opens the same Agent editor as the "Edit" toolbar command - Services and
	// Log now live inside it as MultiPageView pages instead of a separate destination.
	function onEdit(){
		if (agentsView.commandsDelegate){
			agentsView.commandsDelegate.commandHandle("Edit");
		}
	}

	Component {
		id: stateColumnContentComp;
		TableCellDelegateBase {
			id: content

			property string rowStatus: "";

			// Filled pill badge, matching the style previously used on the standalone
			// Enrollment page: reads at a glance for both connection state (Connected/
			// Disconnected/Undefined) and enrollment state (Pending/Suspended/Rejected/
			// Revoked) - CreateRepresentationFromObject only ever puts one or the other here.
			Rectangle {
				id: badge

				anchors.verticalCenter: parent.verticalCenter;
				anchors.left: parent.left;
				anchors.leftMargin: 5;

				width: lable.implicitWidth + 2 * Style.marginM;
				height: lable.implicitHeight + Style.marginS;
				radius: height / 2;
				color: agentsView.colorForStatus(content.rowStatus);
				opacity: 0.14;
			}

			Text {
				id: lable;

				anchors.centerIn: badge;

				font.pixelSize: Style.fontSizeS;
				font.family: Style.fontFamily;
				font.bold: true;
				color: agentsView.colorForStatus(content.rowStatus);

				elide: Text.ElideRight;
			}

			onReused: {
				if (rowIndex >= 0){
					let status = agentsView.table.elements.getData("status", rowIndex);
					content.rowStatus = status !== undefined ? status : "";
				}

				let value = getValue();
				if (value !== undefined){
					lable.text = value;
				}
			}
		}
	}

	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnAgentStatusChanged";

		onMessageReceived: {
			agentsView.doUpdateGui();
			agentsView.refreshStatusCounts();
		}
	}
}
