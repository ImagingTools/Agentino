// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>


namespace agentinogql
{


class CGqlRepresentationAgentDataComp:
			public imtservergql::CGqlRequestHandlerCompBase
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CGqlRepresentationAgentDataComp);
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Parameter providing the client-ID that needs to be identified on the server", false, "ClientIdParam");
	I_END_COMPONENT;

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
};


} // namespace agentinogql


