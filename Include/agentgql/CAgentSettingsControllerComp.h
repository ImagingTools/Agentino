#pragma once


// ImtCore includes
#include <imtcom/IConnectionController.h>
#include <imtgql/CGqlRequestHandlerCompBase.h>
#include <imtbase/IUrlParam.h>


namespace agentgql
{


class CAgentSettingsControllerComp: public imtgql::CGqlRequestHandlerCompBase
{
public:
	typedef imtgql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentSettingsControllerComp);
		I_ASSIGN(m_agentinoUrlCompPtr, "AgentinoUrl", "Agentino URL to enable connectivity", true, "AgentinoUrl");
		I_ASSIGN(m_loginCompPtr, "Login", "Web socket login", true, "WebSocketLogin");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IUrlParam, m_agentinoUrlCompPtr);
	I_REF(imtcom::IConnectionController, m_loginCompPtr);
};


} // namespace agentgql


