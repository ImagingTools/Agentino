// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>


namespace agentinogql
{


class CTopologyControllerComp: public sdl::agentino::Topology::CGraphQlHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTopologyControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Topology collection", true, "TopologyCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
	I_END_COMPONENT;
	
	// reimplemented (sdl::agentino::Topology::CGraphQlHandlerCompBase)
	virtual sdl::agentino::Topology::CTopology OnGetTopology(
				const sdl::agentino::Topology::CGetTopologyGqlRequest& getTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::agentino::Topology::CSaveTopologyResponse OnSaveTopology(
				const sdl::agentino::Topology::CSaveTopologyGqlRequest& saveTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
};


} // namespace agentinodata
