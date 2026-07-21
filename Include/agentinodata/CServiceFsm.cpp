// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceFsm.h>


namespace agentinodata
{


CServiceFsm::TransitionResult CServiceFsm::Apply(ServiceRuntimeStatus from, Event event)
{
	TransitionResult result;
	result.accepted = false;
	result.to = from;
	result.reason = ServiceFailureReason::None;

	switch (from) {
	case ServiceRuntimeStatus::Stopped:
		if (event == Event::Start) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Starting;
		}
		break;

	case ServiceRuntimeStatus::Starting:
		if (event == Event::ChildReady) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Running;
		}
		else if (event == Event::Stop) {
			// Allow cancel-during-start without bypassing the FSM.
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Stopping;
		}
		else if (event == Event::ChildExited) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Failed;
			result.reason = ServiceFailureReason::StartFailed;
		}
		break;

	case ServiceRuntimeStatus::Running:
		if (event == Event::Stop) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Stopping;
		}
		else if (event == Event::ChildExited) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Crashed;
		}
		break;

	case ServiceRuntimeStatus::Stopping:
		if (event == Event::ChildExited) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Stopped;
		}
		else if (event == Event::KillTimeout) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Failed;
			result.reason = ServiceFailureReason::StopTimeout;
		}
		break;

	case ServiceRuntimeStatus::Crashed:
		if (event == Event::RestartBudgetOk) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Starting;
		}
		else if (event == Event::RestartBudgetExhausted) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Failed;
			result.reason = ServiceFailureReason::CrashLooping;
		}
		else if (event == Event::Start) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Starting;
		}
		break;

	case ServiceRuntimeStatus::Failed:
		if (event == Event::Start) {
			result.accepted = true;
			result.to = ServiceRuntimeStatus::Starting;
			result.reason = ServiceFailureReason::None;
		}
		break;
	}

	return result;
}


} // namespace agentinodata
