#pragma once


// Qt includes
#include <QtCore/QTimer>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CObjectCollectionControllerCompBase.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfoRepresentationController.h>


#undef GetObject


namespace agentinogql
{


class CAgentCollectionControllerComp: public QObject, public imtservergql::CObjectCollectionControllerCompBase
{
	Q_OBJECT
public:
	typedef imtservergql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentCollectionControllerComp);
		I_ASSIGN(m_agentFactCompPtr, "AgentFactory", "Factory used for creation of the new agent instance", true, "AgentFactory");
		I_ASSIGN(m_requestHandlerCompPtr, "GqlRequestHandler", "Graphql request handler to create the subscription body", false, "GqlRequestHandler");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
		I_ASSIGN(m_agentStatusCollectionCompPtr, "AgentStatusCollection", "Agent status collection", false, "AgentStatusCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual istd::IChangeable* CreateObjectFromRequest(const imtgql::CGqlRequest& gqlRequest, QByteArray &objectId, QString &name, QString &description, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	void UpdateAgentService(const QByteArray& agentId, const QByteArray& serviceId) const;
	void OnTimeout();

protected:
	I_FACT(agentinodata::IAgentInfo, m_agentFactCompPtr);
	I_REF(imtgql::IGqlRequestHandler, m_requestHandlerCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentStatusCollectionCompPtr);

	mutable QTimer m_timer;
	mutable QList<QByteArray> m_connectedAgents;
	agentinodata::CServiceInfoRepresentationController m_serviceInfoRepresentationController;
};


} // namespace agentinodata


