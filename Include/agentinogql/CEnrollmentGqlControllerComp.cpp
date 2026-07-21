// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CEnrollmentGqlControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Enrollment.h>


// ImtCore includes
#include <imtgql/CGqlRequest.h>


namespace agentinogql
{


namespace
{


/** Wire enum <-> internal enum. Both are generated from the same set of names. */
sdl::V1_0::agentino::EnrollmentStatus ToWireStatus(EnrollmentStatus status)
{
	switch (status){
	case EnrollmentStatus::Pending:
		return sdl::V1_0::agentino::EnrollmentStatus::Pending;
	case EnrollmentStatus::Approved:
		return sdl::V1_0::agentino::EnrollmentStatus::Approved;
	case EnrollmentStatus::Suspended:
		return sdl::V1_0::agentino::EnrollmentStatus::Suspended;
	case EnrollmentStatus::Revoked:
		return sdl::V1_0::agentino::EnrollmentStatus::Revoked;
	case EnrollmentStatus::Rejected:
		return sdl::V1_0::agentino::EnrollmentStatus::Rejected;
	case EnrollmentStatus::Unknown:
	default:
		return sdl::V1_0::agentino::EnrollmentStatus::Unknown;
	}
}


EnrollmentStatus FromWireStatus(sdl::V1_0::agentino::EnrollmentStatus status)
{
	switch (status){
	case sdl::V1_0::agentino::EnrollmentStatus::Pending:
		return EnrollmentStatus::Pending;
	case sdl::V1_0::agentino::EnrollmentStatus::Approved:
		return EnrollmentStatus::Approved;
	case sdl::V1_0::agentino::EnrollmentStatus::Suspended:
		return EnrollmentStatus::Suspended;
	case sdl::V1_0::agentino::EnrollmentStatus::Revoked:
		return EnrollmentStatus::Revoked;
	case sdl::V1_0::agentino::EnrollmentStatus::Rejected:
		return EnrollmentStatus::Rejected;
	case sdl::V1_0::agentino::EnrollmentStatus::Unknown:
	default:
		return EnrollmentStatus::Unknown;
	}
}


QString ToIsoString(const QDateTime& dateTime)
{
	return dateTime.isValid() ? dateTime.toUTC().toString(Qt::ISODateWithMs) : QString();
}


} // namespace


// protected methods

sdl::V1_0::agentino::CEnrollmentAgentListPayload CEnrollmentGqlControllerComp::OnPendingAgentsList(
			const sdl::V1_0::agentino::CPendingAgentsListGqlRequest& /*pendingAgentsListRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	if (!m_enrollmentCompPtr.IsValid()){
		errorMessage = QStringLiteral("EnrollmentController not configured");

		return sdl::V1_0::agentino::CEnrollmentAgentListPayload();
	}

	return ToListPayload(m_enrollmentCompPtr->ListByStatus(EnrollmentStatus::Pending));
}


sdl::V1_0::agentino::CEnrollmentAgentListPayload CEnrollmentGqlControllerComp::OnEnrollmentAgentsList(
			const sdl::V1_0::agentino::CEnrollmentAgentsListGqlRequest& enrollmentAgentsListRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	if (!m_enrollmentCompPtr.IsValid()){
		errorMessage = QStringLiteral("EnrollmentController not configured");

		return sdl::V1_0::agentino::CEnrollmentAgentListPayload();
	}

	const sdl::V1_0::agentino::EnrollmentAgentsListRequestArguments arguments =
				enrollmentAgentsListRequest.GetRequestedArguments();

	// No status filter means "every enrolled agent".
	if (!arguments.input.has_value() || !arguments.input->status.has_value()){
		return ToListPayload(m_enrollmentCompPtr->ListAll());
	}

	return ToListPayload(m_enrollmentCompPtr->ListByStatus(FromWireStatus(*arguments.input->status)));
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::OnApproveAgent(
			const sdl::V1_0::agentino::CApproveAgentGqlRequest& approveAgentRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	const sdl::V1_0::agentino::ApproveAgentRequestArguments arguments =
				approveAgentRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->agentId.has_value()){
		errorMessage = QStringLiteral("ApproveAgent: missing agentId");

		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	const QByteArray agentId = *arguments.input->agentId;
	const QString name = arguments.input->name.has_value() ? *arguments.input->name : QString();
	const QString note = arguments.input->note.has_value() ? *arguments.input->note : QString();

	if (!m_enrollmentCompPtr->Approve(agentId, name, note, errorMessage)){
		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	OnApproved(agentId);

	return MakeMutationPayload(agentId);
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::OnRejectAgent(
			const sdl::V1_0::agentino::CRejectAgentGqlRequest& rejectAgentRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	const sdl::V1_0::agentino::RejectAgentRequestArguments arguments =
				rejectAgentRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->agentId.has_value()){
		errorMessage = QStringLiteral("RejectAgent: missing agentId");

		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	const QByteArray agentId = *arguments.input->agentId;
	const QString note = arguments.input->note.has_value() ? *arguments.input->note : QString();

	if (!m_enrollmentCompPtr->Reject(agentId, note, errorMessage)){
		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	return MakeMutationPayload(agentId);
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::OnRevokeAgent(
			const sdl::V1_0::agentino::CRevokeAgentGqlRequest& revokeAgentRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	const sdl::V1_0::agentino::RevokeAgentRequestArguments arguments =
				revokeAgentRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->agentId.has_value()){
		errorMessage = QStringLiteral("RevokeAgent: missing agentId");

		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	const QByteArray agentId = *arguments.input->agentId;
	const QString note = arguments.input->note.has_value() ? *arguments.input->note : QString();

	if (!m_enrollmentCompPtr->Revoke(agentId, note, errorMessage)){
		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	OnRevoked(agentId);

	return MakeMutationPayload(agentId);
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::OnSuspendAgent(
			const sdl::V1_0::agentino::CSuspendAgentGqlRequest& suspendAgentRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	const sdl::V1_0::agentino::SuspendAgentRequestArguments arguments =
				suspendAgentRequest.GetRequestedArguments();
	if (!arguments.input.has_value()
				|| !arguments.input->agentId.has_value()
				|| !arguments.input->suspend.has_value()){
		errorMessage = QStringLiteral("SuspendAgent: missing agentId/suspend");

		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	const QByteArray agentId = *arguments.input->agentId;
	const bool suspend = *arguments.input->suspend;

	if (!m_enrollmentCompPtr->Suspend(agentId, suspend, errorMessage)){
		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	// Resuming puts the agent back to Approved, so its services must be resynced.
	if (!suspend){
		OnApproved(agentId);
	}

	return MakeMutationPayload(agentId);
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::OnResetRejectedAgent(
			const sdl::V1_0::agentino::CResetRejectedAgentGqlRequest& resetRejectedAgentRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	const sdl::V1_0::agentino::ResetRejectedAgentRequestArguments arguments =
				resetRejectedAgentRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->agentId.has_value()){
		errorMessage = QStringLiteral("ResetRejectedAgent: missing agentId");

		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	const QByteArray agentId = *arguments.input->agentId;
	if (!m_enrollmentCompPtr->ResetRejected(agentId, errorMessage)){
		return sdl::V1_0::agentino::CEnrollmentMutationPayload();
	}

	return MakeMutationPayload(agentId);
}


// private methods

sdl::V1_0::agentino::CEnrollmentAgentItem CEnrollmentGqlControllerComp::ToRepresentation(
			const EnrollmentRecord& record)
{
	sdl::V1_0::agentino::CEnrollmentAgentItem item;
	item.agentId = record.agentId;
	item.status = ToWireStatus(record.status);
	item.claimedName = record.claimedName;
	item.computerName = record.computerName;
	item.advertisedEndpoint = record.advertisedEndpoint;
	item.agentVersion = record.agentVersion;
	item.os = record.os;
	item.firstSeenAt = ToIsoString(record.firstSeenAt);
	item.lastSeenAt = ToIsoString(record.lastSeenAt);
	item.approvedAt = ToIsoString(record.approvedAt);
	item.decisionNote = record.decisionNote;

	return item;
}


sdl::V1_0::agentino::CEnrollmentAgentListPayload CEnrollmentGqlControllerComp::ToListPayload(
			const QVector<EnrollmentRecord>& records)
{
	QList<sdl::V1_0::agentino::CEnrollmentAgentItem> items;
	items.reserve(records.size());
	for (const EnrollmentRecord& record : records){
		items << ToRepresentation(record);
	}

	sdl::V1_0::agentino::CEnrollmentAgentListPayload payload;
	payload.items.Emplace();
	payload.items->FromList(items);

	return payload;
}


sdl::V1_0::agentino::CEnrollmentMutationPayload CEnrollmentGqlControllerComp::MakeMutationPayload(
			const QByteArray& agentId)
{
	sdl::V1_0::agentino::CEnrollmentMutationPayload payload;
	payload.successful = true;
	payload.agentId = agentId;

	return payload;
}


void CEnrollmentGqlControllerComp::OnApproved(const QByteArray& agentId) const
{
	// Async ServicesList reconcile — safe from GQL worker (no nested Wait on this path).
	if (m_serviceSynchronizerCompPtr.IsValid()){
		QString error;
		m_serviceSynchronizerCompPtr->SyncAgentServicesInMirror(agentId, error);
	}
}


void CEnrollmentGqlControllerComp::OnRevoked(const QByteArray& agentId) const
{
	// Nothing to tear down here: a revoked agent stops being ingested by
	// CAgentChangeObserver and CServiceControllerProxy, and its services fall out of
	// the mirror on the next reconcile — which is also what makes its endpoints
	// disappear from the connection pick-list.
	Q_UNUSED(agentId);
}


} // namespace agentinogql
