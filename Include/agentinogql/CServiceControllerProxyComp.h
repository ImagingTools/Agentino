#pragma once


// ImtCore includes
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceInfoRepresentationController.h>


namespace agentinogql
{


class CServiceControllerProxyComp: public imtclientgql::CGqlRemoteRepresentationControllerCompBase
{
public:
	typedef imtclientgql::CGqlRemoteRepresentationControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerProxyComp);
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "ServceManager", true, "ServiceManager");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual agentinodata::IServiceInfo* GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const;
	virtual imtbase::CTreeItemModel* GetRepresentationModelFromServiceInfo(const agentinodata::IServiceInfo& serviceInfo) const;

protected:
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);

private:
	agentinodata::CServiceInfoRepresentationController m_serviceInfoRepresentationController;
};


} // namespace agentinogql


