#pragma once

// ImtCore includes
#include <imtgql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>


namespace agentinogql
{


class CTopologyControllerComp: public imtgql::CGqlRequestHandlerCompBase
{
public:
	typedef imtgql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTopologyControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Topology collection", true, "TopologyCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
	I_END_COMPONENT;

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	imtbase::CTreeItemModel* CreateTopologyModel() const;
	imtbase::CTreeItemModel* SaveTopologyModel(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;
	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	// I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
};


} // namespace agentinodata
