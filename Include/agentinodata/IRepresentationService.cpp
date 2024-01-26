#include <agentinodata/IRepresentationService.h>

// ACF includes
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>

// ImtCore includes
#include <agentino/Version.h>


namespace agentinodata
{


IRepresentationService::ExternProperty::ExternProperty(const QByteArray &id)
{
	if (id.isEmpty()){
		this->id = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}
	else{
		this->id = id;
	}
}


bool IRepresentationService::ExternProperty::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = true;

	iser::CArchiveTag idTag("Id", "Id", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(idTag);
	retVal = retVal && archive.Process(id);
	retVal = retVal && archive.EndTag(idTag);

	iser::CArchiveTag nameTag("Name", "Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(nameTag);
	retVal = retVal && archive.Process(name);
	retVal = retVal && archive.EndTag(nameTag);

	iser::CArchiveTag descriptionTag("Description", "Description", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(descriptionTag);
	retVal = retVal && archive.Process(description);
	retVal = retVal && archive.EndTag(descriptionTag);

	iser::CArchiveTag valueTag("Value", "Value", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(valueTag);
	retVal = retVal && archive.Process(value);
	retVal = retVal && archive.EndTag(valueTag);

	return retVal;
}


IRepresentationService::Link::Link(const QByteArray &id)
{
	if (id.isEmpty()){
		this->id = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}
	else{
		this->id = id;
	}
}


bool IRepresentationService::Link::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = true;

	iser::CArchiveTag idTag("Id", "Id", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(idTag);
	retVal = retVal && archive.Process(id);
	retVal = retVal && archive.EndTag(idTag);

	iser::CArchiveTag nameTag("Name", "Name", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(nameTag);
	retVal = retVal && archive.Process(name);
	retVal = retVal && archive.EndTag(nameTag);

	iser::CArchiveTag descriptionTag("Description", "Description", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(descriptionTag);
	retVal = retVal && archive.Process(description);
	retVal = retVal && archive.EndTag(descriptionTag);

	iser::CArchiveTag agentIdTag("AgentId", "AgentId", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(agentIdTag);
	retVal = retVal && archive.Process(agentId);
	retVal = retVal && archive.EndTag(agentIdTag);

	iser::CArchiveTag serviceIdTag("ServiceId", "ServiceId", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceIdTag);
	retVal = retVal && archive.Process(serviceId);
	retVal = retVal && archive.EndTag(serviceIdTag);

	iser::CArchiveTag propertyIdTag("PropertyId", "PropertyId", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(propertyIdTag);
	retVal = retVal && archive.Process(propertyId);
	retVal = retVal && archive.EndTag(propertyIdTag);

	iser::CArchiveTag serviceTypeIdTag("ServiceTypeId", "ServiceTypeId", iser::CArchiveTag::TT_LEAF);
	retVal = retVal && archive.BeginTag(serviceTypeIdTag);
	retVal = retVal && archive.Process(serviceTypeId);
	retVal = retVal && archive.EndTag(serviceTypeIdTag);

	return retVal;
}


} // namespace agentinodata


