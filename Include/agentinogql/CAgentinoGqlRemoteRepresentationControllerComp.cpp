// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#include <agentinogql/CAgentinoGqlRemoteRepresentationControllerComp.h>


namespace agentinogql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)
bool CAgentinoGqlRemoteRepresentationControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool retVal = BaseClass::IsRequestSupported(gqlRequest);
	if (retVal){
		QByteArray agentId = gqlRequest.GetHeader("clientid");

		if (!agentId.isEmpty()){
			return true;
		}
	}

	return false;
}


} // namespace agentinogql


