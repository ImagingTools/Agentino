#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtgql/CObjectCollectionControllerCompBase.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinodata/IAgentInfo.h>

#undef GetObject


namespace agentinogql
{


class CAgentCollectionControllerComp: public imtgql::CObjectCollectionControllerCompBase
{
public:
	typedef imtgql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentCollectionControllerComp);
		I_ASSIGN(m_agentFactCompPtr, "AgentFactory", "Factory used for creation of the new agent instance", true, "AgentFactory");
		I_ASSIGN(m_requestHandlerPtr, "RequestHandler", "Request handler", true, "RequestHandler");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual istd::IChangeable* CreateObject(const QList<imtgql::CGqlObject>& inputParams, QByteArray &objectId, QString &name, QString &description, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_FACT(agentinodata::IAgentInfo, m_agentFactCompPtr);
	I_REF(imtgql::IGqlRequestHandler, m_requestHandlerPtr);
};


} // namespace agentinodata


