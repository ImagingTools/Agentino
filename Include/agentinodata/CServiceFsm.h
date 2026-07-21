// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/ServiceRuntimeState.h>


namespace agentinodata
{


/**
	Normative service FSM transition table (Architecture Audit §4.6).
	Pure logic — no OS or I/O.
*/
class CServiceFsm
{
public:
	enum class Event
	{
		Start,
		Stop,
		ChildReady,
		ChildExited,
		KillTimeout,
		RestartBudgetOk,
		RestartBudgetExhausted
	};

	struct TransitionResult
	{
		bool accepted = false;
		ServiceRuntimeStatus to = ServiceRuntimeStatus::Stopped;
		ServiceFailureReason reason = ServiceFailureReason::None;
	};

	static TransitionResult Apply(ServiceRuntimeStatus from, Event event);
};


} // namespace agentinodata
