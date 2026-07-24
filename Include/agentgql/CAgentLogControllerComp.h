// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Generated includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents_fwd.h>


namespace agentgql
{


class CAgentLogControllerComp: public imtservergql::CGqlRequestHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentLogControllerComp);
		I_ASSIGN(m_agentLogCollectionCompPtr, "AgentLogCollection", "Agent log collection", true, "AgentLogCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtservergql::CGqlRequestHandlerCompBase)
	virtual QJsonObject CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_agentLogCollectionCompPtr);
};


} // namespace agentgql