// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QTimer>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents.h>


#undef GetObject


namespace agentinogql
{


class CAgentCollectionControllerComp: public QObject, public imtclientgql::TClientRequestManagerCompWrap<sdl::agentino::Agents::CAgentCollectionControllerCompBase>
{
	Q_OBJECT
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<sdl::agentino::Agents::CAgentCollectionControllerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CAgentCollectionControllerComp);
		I_ASSIGN(m_agentFactCompPtr, "AgentFactory", "Factory used for creation of the new agent instance", true, "AgentFactory");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection", false, "AgentStatusCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	
	// reimplemented (sdl::agentino::Agents::CAgentCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::agentino::Agents::CAgentsListGqlRequest& agentsListRequest,
				sdl::agentino::Agents::CAgentItem::V1_0& representationObject,
				QString& errorMessage) const override;
	virtual bool CreateRepresentationFromObject(
				const istd::IChangeable& data,
				const sdl::agentino::Agents::CGetAgentGqlRequest& agentItemRequest,
				sdl::agentino::Agents::CAgentData::V1_0& representationPayload,
				QString& errorMessage) const override;
	virtual bool UpdateObjectFromRepresentationRequest(
				const ::imtgql::CGqlRequest& rawGqlRequest,
				const sdl::agentino::Agents::CUpdateAgentGqlRequest& updateAgentRequest,
				istd::IChangeable& object,
				QString& errorMessage) const override;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual istd::IChangeableUniquePtr CreateObjectFromRequest(
				const imtgql::CGqlRequest& gqlRequest,
				QByteArray &objectId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* InsertObject(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
private:
	bool UpdateServiceStatusFromAgent(const QByteArray& agentId, const QByteArray& serviceId) const;
	void OnTimeout();

protected:
	I_FACT(agentinodata::IAgentInfo, m_agentFactCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);

	mutable QTimer m_timer;
	mutable QList<QByteArray> m_connectedAgents;
};


} // namespace agentinodata


