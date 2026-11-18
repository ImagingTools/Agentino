// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentinogql
{


/**
	GraphQL surface of the "Update" page: the service types (software products) installed on the
	agents, with the number of agents using each of them.

	The schema lives in Sdl/agentino/1.0/Updates.sdl.
*/
class CServiceUpdateControllerComp: public sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceUpdateControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections", true, "ServiceManager");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CServiceUpdateListPayload OnServiceUpdatesList(
				const sdl::V1_0::agentino::CServiceUpdatesListGqlRequest& serviceUpdatesListRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		Get the number of agents using each service, by service type ID.
		An agent with several services of the same type is counted once.
		A service without a type is counted under its own name.
	*/
	QMap<QString, int> GetServiceAgentCounts() const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
};


} // namespace agentinogql
