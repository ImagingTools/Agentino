// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CSoftwareUpdateCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// ACF includes
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/IObjectCollectionIterator.h>

// Agentino includes
#include <agentinodata/CSoftwareUpdateInfo.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CSoftwareUpdateCollectionControllerCompBase)

bool CSoftwareUpdateCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::V1_0::agentino::CAvailableUpdatesGqlRequest& /*availableUpdatesRequest*/,
			sdl::V1_0::agentino::CUpdateItem& representationObject,
			QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CSoftwareUpdateCollectionControllerComp");
		return false;
	}

	imtbase::IObjectCollection::DataPtr dataPtr = objectCollectionIterator.GetObjectData();
	if (dataPtr.IsNull()){
		errorMessage = "Unable to get update data";
		return false;
	}

	const agentinodata::ISoftwareUpdateInfo* updateInfoPtr = dynamic_cast<const agentinodata::ISoftwareUpdateInfo*>(dataPtr.GetPtr());
	if (updateInfoPtr == nullptr){
		errorMessage = "Invalid update info object";
		return false;
	}

	representationObject.id = objectCollectionIterator.GetObjectId();
	representationObject.typeId = m_objectCollectionCompPtr->GetObjectTypeId(objectCollectionIterator.GetObjectId());
	representationObject.name = updateInfoPtr->GetName();
	representationObject.version = updateInfoPtr->GetVersion();
	representationObject.updateType = static_cast<sdl::V1_0::agentino::UpdateType>(updateInfoPtr->GetUpdateType());
	representationObject.description = updateInfoPtr->GetDescription();
	representationObject.fileSize = static_cast<int>(updateInfoPtr->GetFileSize());
	representationObject.checksum = updateInfoPtr->GetChecksum();
	representationObject.publishedDate = updateInfoPtr->GetPublishedDate();
	representationObject.status = static_cast<sdl::V1_0::agentino::UpdateStatus>(updateInfoPtr->GetStatus());

	return true;
}


bool CSoftwareUpdateCollectionControllerComp::CreateRepresentationFromObject(
			const istd::IChangeable& data,
			const sdl::V1_0::agentino::CGetUpdateInfoGqlRequest& getUpdateInfoRequest,
			sdl::V1_0::agentino::CUpdateData& updateData,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::GetUpdateInfoRequestArguments arguments = getUpdateInfoRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->id.has_value()){
		errorMessage = "Update ID is missing in the request";
		return false;
	}

	const agentinodata::ISoftwareUpdateInfo* updateInfoPtr = dynamic_cast<const agentinodata::ISoftwareUpdateInfo*>(&data);
	if (updateInfoPtr == nullptr){
		errorMessage = "Invalid update info object";
		return false;
	}

	updateData.id = *arguments.input->id;
	updateData.name = updateInfoPtr->GetName();
	updateData.version = updateInfoPtr->GetVersion();
	updateData.updateType = static_cast<sdl::V1_0::agentino::UpdateType>(updateInfoPtr->GetUpdateType());
	updateData.description = updateInfoPtr->GetDescription();
	updateData.fileSize = static_cast<int>(updateInfoPtr->GetFileSize());
	updateData.checksum = updateInfoPtr->GetChecksum();
	updateData.publishedDate = updateInfoPtr->GetPublishedDate();
	updateData.status = static_cast<sdl::V1_0::agentino::UpdateStatus>(updateInfoPtr->GetStatus());
	updateData.changelog = updateInfoPtr->GetChangelog();
	updateData.targetVersion = updateInfoPtr->GetTargetVersion();
	updateData.sourceUrl = updateInfoPtr->GetSourceUrl();

	return true;
}


} // namespace agentgql


