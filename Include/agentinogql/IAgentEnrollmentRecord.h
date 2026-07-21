// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <iser/IObject.h>

// Agentino includes
#include <agentinogql/EnrollmentTypes.h>


namespace agentinogql
{


/**
	One agent's enrollment record, as a collection item of EnrollmentStoreComp's
	RecordCollection (see EnrollmentRecordCollection in ServerBase.acc).

	agentId is deliberately not part of the serialized payload: it is the collection's
	own element id (see AgentInfo/AgentStatusInfo for the same convention), so GetRecord()
	needs it passed in rather than reading it back out of itself.
*/
class IAgentEnrollmentRecord: virtual public iser::IObject
{
public:
	/** \param agentId Not stored on this object - the collection's own element id. */
	virtual EnrollmentRecord GetRecord(const QByteArray& agentId) const = 0;

	/** \a record.agentId is ignored - the collection's element id is what's authoritative. */
	virtual void SetRecord(const EnrollmentRecord& record) = 0;
};


} // namespace agentinogql
