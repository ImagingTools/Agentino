// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtcom/IConnectionController.h>
#include <imtservergql/CGqlRequestHandlerCompBase.h>
#include <imtcom/IServerConnectionInterface.h>


namespace agentgql
{


class CAgentSettingsControllerComp: public imtservergql::CGqlRequestHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentSettingsControllerComp);
		I_ASSIGN(m_agentinoConnectionInterfaceCompPtr, "AgentinoConnectionInterface", "Agentino ConnectionInterface to enable connectivity", true, "AgentinoConnectionInterface");
		I_ASSIGN(m_loginCompPtr, "Login", "Web socket login", true, "WebSocketLogin");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtcom::IServerConnectionInterface, m_agentinoConnectionInterfaceCompPtr);
	I_REF(imtcom::IConnectionController, m_loginCompPtr);
};


} // namespace agentgql


