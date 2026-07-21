// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinogql/IEnrollmentController.h>
#include <agentinogql/IServiceCollectionSynchronizer.h>

#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Enrollment_fwd.h>


namespace agentinogql
{


/**
	GraphQL admin surface for agent enrollment (Architecture §4.13).

	The schema lives in Sdl/agentino/1.0/Enrollment.sdl; this component only implements
	the generated handlers. Decisions are not attributed to any operator — the system
	has no notion of a user, an approval is simply recorded against the agent.
*/
class CEnrollmentGqlControllerComp: public sdl::V1_0::agentino::CEnrollmentGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CEnrollmentGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CEnrollmentGqlControllerComp);
		I_ASSIGN(m_enrollmentCompPtr, "EnrollmentController", "Sole writer of enrollment records", true, "EnrollmentStore");
		I_ASSIGN(m_serviceSynchronizerCompPtr, "ServiceSynchronizer", "Resync services after approve", false, "ServiceSynchronizer");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CEnrollmentGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CEnrollmentAgentListPayload OnPendingAgentsList(
				const sdl::V1_0::agentino::CPendingAgentsListGqlRequest& pendingAgentsListRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentAgentListPayload OnEnrollmentAgentsList(
				const sdl::V1_0::agentino::CEnrollmentAgentsListGqlRequest& enrollmentAgentsListRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentMutationPayload OnApproveAgent(
				const sdl::V1_0::agentino::CApproveAgentGqlRequest& approveAgentRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentMutationPayload OnRejectAgent(
				const sdl::V1_0::agentino::CRejectAgentGqlRequest& rejectAgentRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentMutationPayload OnRevokeAgent(
				const sdl::V1_0::agentino::CRevokeAgentGqlRequest& revokeAgentRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentMutationPayload OnSuspendAgent(
				const sdl::V1_0::agentino::CSuspendAgentGqlRequest& suspendAgentRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CEnrollmentMutationPayload OnResetRejectedAgent(
				const sdl::V1_0::agentino::CResetRejectedAgentGqlRequest& resetRejectedAgentRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	static sdl::V1_0::agentino::CEnrollmentAgentItem ToRepresentation(const EnrollmentRecord& record);
	static sdl::V1_0::agentino::CEnrollmentMutationPayload MakeMutationPayload(const QByteArray& agentId);
	static sdl::V1_0::agentino::CEnrollmentAgentListPayload ToListPayload(
				const QVector<EnrollmentRecord>& records);

	/** Resync the agent's services once it is allowed to talk to us again. */
	void OnApproved(const QByteArray& agentId) const;
	/** Drop the agent's endpoints from the catalog once it is no longer trusted. */
	void OnRevoked(const QByteArray& agentId) const;

private:
	I_REF(IEnrollmentController, m_enrollmentCompPtr);
	I_REF(IServiceCollectionSynchronizer, m_serviceSynchronizerCompPtr);
};


} // namespace agentinogql
