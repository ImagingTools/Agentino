// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CGqlRequestHandlerCompBase.h>

// Agentino includes
#include <agentinodata/ISoftwareUpdateManager.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentgql
{


/**
	Controller component handling update mutations (ApplyUpdate, RollbackUpdate).
*/
class CSoftwareUpdateControllerComp: public sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CSoftwareUpdateControllerComp);
		I_ASSIGN(m_updateManagerCompPtr, "UpdateManager", "Update manager for applying and rolling back updates", true, "UpdateManager");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CApplyUpdateResponse OnApplyUpdate(
				const sdl::V1_0::agentino::CApplyUpdateGqlRequest& applyUpdateRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CRollbackUpdateResponse OnRollbackUpdate(
				const sdl::V1_0::agentino::CRollbackUpdateGqlRequest& rollbackUpdateRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	I_REF(agentinodata::ISoftwareUpdateManager, m_updateManagerCompPtr);
};


} // namespace agentgql


