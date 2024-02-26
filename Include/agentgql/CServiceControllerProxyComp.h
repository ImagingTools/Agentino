#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentgql
{


class CServiceControllerProxyComp: public imtclientgql::CGqlRemoteRepresentationControllerCompBase
{
public:
	typedef imtclientgql::CGqlRemoteRepresentationControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerProxyComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual agentinodata::IServiceInfo* GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const;
	virtual imtbase::CTreeItemModel* GetRepresentationModelFromServiceInfo(const agentinodata::IServiceInfo& serviceInfo) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentgql


