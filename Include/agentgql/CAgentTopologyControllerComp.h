// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentTopology_fwd.h>


namespace agentgql
{


/**
	Agent-side topology controller that exposes local service topology
	via the AgentTopology SDL schema. This component makes the agent
	the source of truth for its own services.
*/
class CAgentTopologyControllerComp: public sdl::V1_0::agentino::CAgentTopologyGqlHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentTopologyControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", true, "ServiceCollection");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", false, "ServiceController");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Topology collection for storing coordinates", false, "TopologyCollection");
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Parameter providing the agent client-ID", false, "ClientIdParam");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CAgentTopologyGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CLocalTopology OnGetLocalTopology(
				const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& getLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CLocalServiceStatusResponse OnGetLocalServiceStatus(
				const sdl::V1_0::agentino::CGetLocalServiceStatusGqlRequest& getLocalServiceStatusRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSaveLocalTopologyResponse OnSaveLocalTopology(
				const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& saveLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;
	sdl::V1_0::agentino::LocalServiceStatus GetLocalServiceStatusEnum(const QByteArray& serviceId) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
};


} // namespace agentgql
