// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>
#include <QtCore/QPoint>

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CTopologyView.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology_fwd.h>


namespace agentinogql
{


class CTopologyControllerComp: public sdl::V1_0::agentino::CTopologyGqlHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CTopologyControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections (R1.3)", false, "ServiceManager");
		I_ASSIGN(m_topologyCollectionCompPtr, "TopologyCollection", "Topology collection", true, "TopologyCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection used to mark services of offline agents", false, "AgentStatusCollection");
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
	virtual sdl::V1_0::agentino::CCreateTopologyGroupResponse OnCreateTopologyGroup(
				const sdl::V1_0::agentino::CCreateTopologyGroupGqlRequest& createTopologyGroupRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnMoveTopologyNodes(
				const sdl::V1_0::agentino::CMoveTopologyNodesGqlRequest& moveTopologyNodesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnRemoveTopologyGroup(
				const sdl::V1_0::agentino::CRemoveTopologyGroupGqlRequest& removeTopologyGroupRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnRenameTopologyGroup(
				const sdl::V1_0::agentino::CRenameTopologyGroupGqlRequest& renameTopologyGroupRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnRemoveFromTopologyView(
				const sdl::V1_0::agentino::CRemoveFromTopologyViewGqlRequest& removeFromTopologyViewRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnAddToTopologyView(
				const sdl::V1_0::agentino::CAddToTopologyViewGqlRequest& addToTopologyViewRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyViewList OnGetTopologyViews(
				const sdl::V1_0::agentino::CGetTopologyViewsGqlRequest& getTopologyViewsRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CCreateTopologyViewResponse OnCreateTopologyView(
				const sdl::V1_0::agentino::CCreateTopologyViewGqlRequest& createTopologyViewRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CTopologyGroupResponse OnRemoveTopologyView(
				const sdl::V1_0::agentino::CRemoveTopologyViewGqlRequest& removeTopologyViewRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	struct TopologyGroupInfo
	{
		QString name;
		QByteArrayList memberIds;
	};

	typedef QMap<QByteArray, TopologyGroupInfo> TopologyGroupMap;

	QPoint GetServiceCoordinate(const QByteArray& serviceId) const;
	bool SetServiceCoordinate(const QByteArray& serviceId, const QPoint& point) const;
	bool IsAgentOnline(const QByteArray& agentId) const;

	/**
		Get IDs of the user-defined views in the order of the topology collection.
	*/
	QByteArrayList GetTopologyViewIds() const;

	/**
		Get the user-defined view. Returns false if the view does not exist.
	*/
	bool GetTopologyView(const QByteArray& viewId, agentinodata::CTopologyView& view) const;

	/**
		Get the view the group belongs to. Empty for the default view (grouping by agents).
	*/
	QByteArray FindGroupViewId(const QByteArray& groupId) const;

	/**
		Get the groups of the view. An empty view ID stands for the default view (grouping by agents),
		which owns the groups not listed in any user-defined view.
	*/
	TopologyGroupMap GetTopologyGroups(const QByteArray& viewId) const;
	bool SetTopologyGroupMembers(const QByteArray& groupId, const QByteArrayList& memberIds) const;

	/**
		Get the group directly containing the node. Empty for nodes outside of all groups.
	*/
	static QByteArray FindParentGroupId(const QByteArray& nodeId, const TopologyGroupMap& groups);

	/**
		Check whether the group is the given ancestor group or is nested in it.
	*/
	static bool IsGroupInside(const QByteArray& groupId, const QByteArray& ancestorGroupId, const TopologyGroupMap& groups);

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(imtbase::IObjectCollection, m_topologyCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);
};


} // namespace agentinogql
