// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CObjectCollectionControllerCompBase.h>

// Agentino includes
#include <agentinodata/CUpdateInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentgql
{


class CUpdateCollectionControllerComp:
			public sdl::V1_0::agentino::CUpdateCollectionControllerCompBase
{
public:
	typedef CUpdateCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CUpdateCollectionControllerComp);
		I_ASSIGN(m_updateInfoFactCompPtr, "UpdateFactory", "Factory used for creation of the new update info instance", false, "UpdateFactory");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CUpdateCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::V1_0::agentino::CAvailableUpdatesGqlRequest& availableUpdatesRequest,
				sdl::V1_0::agentino::CUpdateItem& representationObject,
				QString& errorMessage) const override;
	virtual bool CreateRepresentationFromObject(
				const istd::IChangeable& data,
				const sdl::V1_0::agentino::CGetUpdateInfoGqlRequest& getUpdateInfoRequest,
				sdl::V1_0::agentino::CUpdateData& updateData,
				QString& errorMessage) const override;

protected:
	I_FACT(agentinodata::IUpdateInfo, m_updateInfoFactCompPtr);
};


} // namespace agentgql


