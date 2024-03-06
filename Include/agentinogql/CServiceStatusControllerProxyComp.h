#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>


namespace agentinogql
{


class CServiceStatusControllerProxyComp: public imtclientgql::CGqlRemoteRepresentationControllerCompBase
{
public:
	typedef imtclientgql::CGqlRemoteRepresentationControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceStatusControllerProxyComp);
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", true, "ServiceStatusCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
};


} // namespace agentinogql


