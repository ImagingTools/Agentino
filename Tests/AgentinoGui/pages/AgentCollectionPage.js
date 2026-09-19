// AgentCollectionPage - the "Agents" page (AgentCollectionView.qml / AgentCollectionViewBase.qml).
//
// Not a standard Imt collection: it has no server-side sort (hasSort: false) and its "status" column
// is a live, computed field the generic collection-filter machinery cannot see
// (CAgentCollectionControllerComp::ListObjects filters on a plain "status" input param instead). So
// filtering by status is six dedicated toolbar buttons (AgentStatusFilterDecorator.qml: All / Pending /
// Approved / Suspended / Rejected / Revoked - each carries its live count in its own text, e.g.
// "Pending (2)", which is why each got a stable objectName of its own instead of relying on the
// text-derived default), not the FilterPanel combo CollectionPage assumes.
//
// Enrollment is a workflow over the same six statuses, driven from AgentCollectionViewCommandsDelegate.qml
// (only one decision command is enabled at a time, depending on the selected row's status):
//   Pending             -> Approve | Reject
//   Approved            -> Edit | Suspend | Revoke
//   Suspended           -> Resume | Revoke
//   Rejected | Revoked  -> Reset (back to Pending)
// "Edit" only opens the AgentEditor for an Approved agent - the commands delegate's onEdit() is a
// silent no-op otherwise, matching the toolbar itself being disabled.

const gui = require('imtcore-gui-testkit/lib/gui');
const { CollectionPage } = require('imtcore-gui-testkit/pages/CollectionPage');

// Agents.acc's FilterableHeaderIds (name/computerName/description/services/status/version) plus the
// live lastConnection column - none of these are used by this suite via the generic FilterPanel path
// (see class doc above), but the header ids themselves are still the right thing to mask.
//
// "status" is deliberately NOT masked. It was, briefly, while the test agent was unroutable and the
// column flipped between "Connected" and "Disconnected" - but that was the bug (an empty ClientId; see
// Run-CiTests.ps1), not a property of the column. With the agent routed, an approved agent reads
// "Connected" for the whole run, so masking it would only hide a regression of exactly that bug.
// "version" is the agent's own build number, which moves with every rebuild (1.0.0.549 -> 1.0.0.556
// between two runs here), so it is masked for the same reason lastConnection is.
// "name" and "computerName" both hold the machine's own name ("LAPTOP" here,
// "b035a0a.online-server.cloud" on the build agent), so they are masked too - without that this
// baseline only ever matches the machine that generated it.
const MASK_COLUMNS = ['lastConnection', 'version', 'name', 'computerName'];

const STATUS_FILTERS = {
  all: 'AgentStatusAllFilter',
  pending: 'AgentStatusPendingFilter',
  approved: 'AgentStatusApprovedFilter',
  suspended: 'AgentStatusSuspendedFilter',
  rejected: 'AgentStatusRejectedFilter',
  revoked: 'AgentStatusRevokedFilter',
};

class AgentCollectionPage extends CollectionPage {
  constructor(page) {
    super(page, 'Agents', { maskColumns: MASK_COLUMNS });
  }

  /** Click one of the six status toolbar buttons (see STATUS_FILTERS keys). */
  filterByStatus(bucket) {
    const objectName = STATUS_FILTERS[bucket];
    if (!objectName) throw new Error(`Unknown agent status bucket "${bucket}" - expected one of ${Object.keys(STATUS_FILTERS)}`);
    return gui.clickButton(this.page, [objectName]);
  }

  approve() {
    return this.runCommand('Approve');
  }
  reject() {
    return this.runCommand('Reject');
  }
  suspend() {
    return this.runCommand('Suspend');
  }
  resume() {
    return this.runCommand('Resume');
  }
  revoke() {
    return this.runCommand('Revoke');
  }
  reset() {
    return this.runCommand('Reset');
  }
}

module.exports = { AgentCollectionPage, STATUS_FILTERS, MASK_COLUMNS };
