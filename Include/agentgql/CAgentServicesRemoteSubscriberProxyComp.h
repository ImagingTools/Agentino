#pragma once

// ImtCore includes
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>
#include <imtservice/IConnectionCollectionProvider.h>
#include <imtgql/CGqlSubscriberControllerCompBase.h>


namespace agentgql
{


class CAgentServicesRemoteSubscriberProxyComp: public imtgql::CGqlSubscriberControllerCompBase
{
public:
	typedef imtgql::CGqlSubscriberControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentServicesRemoteSubscriberProxyComp)
		I_ASSIGN(m_connectionCollectionProviderCompPtr, "ConnectionCollectionProvider", "Application info", true, "ConnectionCollectionProvider");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;

	// // reimplemented (imtgql::CGqlRepresentationDataControllerComp)
	// virtual imtbase::CTreeItemModel* CreateInternalResponse(
	// 	const imtgql::CGqlRequest& gqlRequest,
	// 	QString& errorMessage) const override;

protected:
	I_REF(imtservice::IConnectionCollectionProvider, m_connectionCollectionProviderCompPtr);
};


} // namespace agentgql


