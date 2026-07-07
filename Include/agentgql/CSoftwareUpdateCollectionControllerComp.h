// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CObjectCollectionControllerCompBase.h>

// Agentino includes
#include <agentinodata/CSoftwareUpdateInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentgql
{


class CSoftwareUpdateCollectionControllerComp:
			public sdl::V1_0::agentino::CSoftwareUpdateCollectionControllerCompBase
{
public:
	typedef CSoftwareUpdateCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CSoftwareUpdateCollectionControllerComp);
		I_ASSIGN(m_updateInfoFactCompPtr, "UpdateFactory", "Factory used for creation of the new update info instance", false, "UpdateFactory");
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CSoftwareUpdateCollectionControllerCompBase)
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
	I_FACT(agentinodata::ISoftwareUpdateInfo, m_updateInfoFactCompPtr);
};


} // namespace agentgql


