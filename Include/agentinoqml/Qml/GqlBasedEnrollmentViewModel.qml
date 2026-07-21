import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0
import agentinoEnrollmentSdl 1.0

/**
	L3 adapter: EnrollmentViewModel over Enrollment.sdl admin mutations
	(ApproveAgent, RejectAgent, RevokeAgent, SuspendAgent, ResetRejectedAgent)
	via GqlSdlRequestSender + generated SDL input/payload types.
*/
EnrollmentViewModel {
	id: root

	property ApproveAgentInput approveAgentInput: ApproveAgentInput {}
	property EnrollmentDecisionInput rejectAgentInput: EnrollmentDecisionInput {}
	property EnrollmentDecisionInput revokeAgentInput: EnrollmentDecisionInput {}
	property SuspendAgentInput suspendAgentInput: SuspendAgentInput {}
	property AgentIdInput resetRejectedAgentInput: AgentIdInput {}

	function beginDecision(){
		root.loading = true
		root.errorMessage = ""
	}

	function endDecisionOk(){
		root.loading = false
		root.decisionFinished()
	}

	function endDecisionError(commandId){
		root.loading = false
		root.errorMessage = qsTr("%1 failed").arg(commandId)
		root.decisionFinished()
	}

	function mutationRequestedFields(){
		var fields = Gql.GqlObject("payload")
		fields.InsertField("successful")
		fields.InsertField("agentId")
		return fields
	}

	// Port signal handlers → SDL mutations
	onApproveAgent: {
		root.beginDecision()
		approveAgentInput.m_agentId = agentId
		approveAgentInput.m_name = name
		approveAgentInput.m_note = note
		approveAgentRequestSender.send(approveAgentInput)
	}

	onRejectAgent: {
		root.beginDecision()
		rejectAgentInput.m_agentId = agentId
		rejectAgentInput.m_note = note
		rejectAgentRequestSender.send(rejectAgentInput)
	}

	onRevokeAgent: {
		root.beginDecision()
		revokeAgentInput.m_agentId = agentId
		revokeAgentInput.m_note = note
		revokeAgentRequestSender.send(revokeAgentInput)
	}

	onSuspendAgent: {
		root.beginDecision()
		suspendAgentInput.m_agentId = agentId
		suspendAgentInput.m_suspend = suspend
		suspendAgentRequestSender.send(suspendAgentInput)
	}

	onResetRejectedAgent: {
		root.beginDecision()
		resetRejectedAgentInput.m_agentId = agentId
		resetRejectedAgentRequestSender.send(resetRejectedAgentInput)
	}

	onRequestAgentsList: {
		root.listLoading = true
		root.errorMessage = ""
		// GqlSdlRequestSender.send(inputObject) serializes EVERY m_-prefixed property of
		// inputObject unconditionally (Qml/web/GraphQLRequest.js: GqlObject.fromObject()
		// loops object.getProperties() and calls InsertField for each, with no concept of
		// "unset"). So an omitted enum field still goes out as status:"" and the server
		// rejects it ("" is not a valid EnrollmentStatus) - this holds regardless of
		// m_status's own value or any removeStatus()/removeKey() call, since fromObject()
		// never consults that bookkeeping at all. Bypass it: build the input params by
		// hand, so "status" is only present in the request when actually filtering.
		agentsListRequestSender.inputParams = Gql.GqlObject("input")
		if (status && status.length > 0){
			agentsListRequestSender.addInputParam("status", status)
		}
		agentsListRequestSender.send()
	}

	property GqlSdlRequestSender agentsListRequestSender: GqlSdlRequestSender {
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_enrollmentAgentsList

		onFinished: {
			root.listLoading = false
			if (status === -1){
				root.errorMessage = qsTr("Failed to load agents")
				root.agentsList = null
				root.agentsListLoaded()
			}
		}

		sdlObjectComp: Component {
			EnrollmentAgentListPayload {
				onFinished: {
					root.agentsList = (hasItems && hasItems()) ? m_items : null
					root.agentsListLoaded()
				}
			}
		}
	}

	property GqlSdlRequestSender approveAgentRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_approveAgent

		function getRequestedFields(){
			return root.mutationRequestedFields()
		}

		onFinished: {
			if (status === -1)
				root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_approveAgent)
		}

		sdlObjectComp: Component {
			EnrollmentMutationPayload {
				onFinished: {
					if (!m_successful){
						root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_approveAgent)
						return
					}
					root.endDecisionOk()
				}
			}
		}
	}

	property GqlSdlRequestSender rejectAgentRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_rejectAgent

		function getRequestedFields(){
			return root.mutationRequestedFields()
		}

		onFinished: {
			if (status === -1)
				root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_rejectAgent)
		}

		sdlObjectComp: Component {
			EnrollmentMutationPayload {
				onFinished: {
					if (!m_successful){
						root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_rejectAgent)
						return
					}
					root.endDecisionOk()
				}
			}
		}
	}

	property GqlSdlRequestSender revokeAgentRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_revokeAgent

		function getRequestedFields(){
			return root.mutationRequestedFields()
		}

		onFinished: {
			if (status === -1)
				root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_revokeAgent)
		}

		sdlObjectComp: Component {
			EnrollmentMutationPayload {
				onFinished: {
					if (!m_successful){
						root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_revokeAgent)
						return
					}
					root.endDecisionOk()
				}
			}
		}
	}

	property GqlSdlRequestSender suspendAgentRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_suspendAgent

		function getRequestedFields(){
			return root.mutationRequestedFields()
		}

		onFinished: {
			if (status === -1)
				root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_suspendAgent)
		}

		sdlObjectComp: Component {
			EnrollmentMutationPayload {
				onFinished: {
					if (!m_successful){
						root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_suspendAgent)
						return
					}
					root.endDecisionOk()
				}
			}
		}
	}

	property GqlSdlRequestSender resetRejectedAgentRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoEnrollmentSdlCommandIds.s_resetRejectedAgent

		function getRequestedFields(){
			return root.mutationRequestedFields()
		}

		onFinished: {
			if (status === -1)
				root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_resetRejectedAgent)
		}

		sdlObjectComp: Component {
			EnrollmentMutationPayload {
				onFinished: {
					if (!m_successful){
						root.endDecisionError(AgentinoEnrollmentSdlCommandIds.s_resetRejectedAgent)
						return
					}
					root.endDecisionOk()
				}
			}
		}
	}

}
