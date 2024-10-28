#pragma once


// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceController.h>



namespace agentgql
{


class CServiceControllerComp: public imtservergql::CGqlRequestHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

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


