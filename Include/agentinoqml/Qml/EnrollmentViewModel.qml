import QtQuick 2.12

/**
	L2 port: agent enrollment decisions (Architecture Audit §6.4 / §4.13).
	Transport-agnostic; mutate via an L3 adapter.
*/
QtObject {
	id: root

	property bool loading: false
	property string errorMessage: ""

	signal approveAgent(string agentId, string name, string note)
	signal rejectAgent(string agentId, string note)
	signal revokeAgent(string agentId, string note)
	signal suspendAgent(string agentId, bool suspend)
	signal resetRejectedAgent(string agentId)
	// Fired after a decision mutation finishes (success or failure) so the UI can reload.
	signal decisionFinished()

	function requestApprove(agentId, name, note) { approveAgent(agentId, name || "", note || "") }
	function requestReject(agentId, note) { rejectAgent(agentId, note || "") }
	function requestRevoke(agentId, note) { revokeAgent(agentId, note || "") }
	function requestSuspend(agentId, suspend) { suspendAgent(agentId, suspend) }
	function requestResetRejected(agentId) { resetRejectedAgent(agentId || "") }

	// Enrollment list (Pending/Approved/Suspended/Rejected/Revoked/"" for all).
	property bool listLoading: false
	property var agentsList: null

	signal requestAgentsList(string status)
	signal agentsListLoaded()

	function requestList(status) { requestAgentsList(status || "") }
}
