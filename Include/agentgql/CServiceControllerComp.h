#pragma once


// ImtCore includes
#include <imtgql/CGqlRequestHandlerCompBase.h>

// ServiceManager includes
#include <agentinodata/IServiceController.h>



namespace agentgql
{


class CServiceControllerComp: public imtgql::CGqlRequestHandlerCompBase
{
public:
	typedef imtgql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerComp);
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", true, "ServiceController");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
};


} // namespace agentgql


