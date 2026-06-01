// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology_fwd.h>


namespace agentgql
{


/**
	Agent-side topology snapshot controller.

	Serves GetTopology by reading the agent's local service collection,
	building a snapshot with service metadata, status, and dependency links.
	This makes each agent the source of truth for its own topology data.

	SaveTopology is not supported on the agent side (layout coordinates
	are managed centrally by the server).
*/
class CTopologySnapshotControllerComp: public sdl::V1_0::agentino::CTopologyGqlHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTopologySnapshotControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ObjectCollection", "Service collection", true, "ObjectCollection");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller for status queries", false, "ServiceController");
	I_END_COMPONENT;

	// reimplemented (sdl::V1_0::agentino::CTopologyGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CTopology OnGetTopology(
				const sdl::V1_0::agentino::CGetTopologyGqlRequest& getTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSaveTopologyResponse OnSaveTopology(
				const sdl::V1_0::agentino::CSaveTopologyGqlRequest& saveTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	QByteArray ResolveServiceIdByConnectionId(const QByteArray& connectionId) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
};


} // namespace agentgql
