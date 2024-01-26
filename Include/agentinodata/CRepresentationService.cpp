#include <agentinodata/CRepresentationService.h>


// Qt includes
#include <QtCore/QByteArrayList>

// ACF includes
#include <istd/TDelPtr.h>
#include <istd/CChangeNotifier.h>
#include <iser/IArchive.h>
#include <iser/CArchiveTag.h>
#include <iser/CPrimitiveTypesSerializer.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <agentino/Version.h>


namespace agentinodata
{


// public methods

CRepresentationService::CRepresentationService(ServiceType serviceType):
			CServiceInfo(serviceType)
{
}


QPointF CRepresentationService::GetPoint() const
{
	return m_point;
}


void CRepresentationService::SetPoint(const QPointF& point)
{
	m_point = point;
}


IRepresentationService::PropertyList CRepresentationService::GetExternPropertyList() const
{
	return m_propertyList;
}


void CRepresentationService::SetExternPropertyList(const PropertyList& propertyList)
{
	m_propertyList = propertyList;
}


IRepresentationService::LinkList CRepresentationService::GetLinkList() const
{
	return m_linkList;
}


void CRepresentationService::SetLinkList(const LinkList& linkList)
{
	m_linkList = linkList;
}


bool CRepresentationService::Serialize(iser::IArchive &archive)
{
	// Get ImtCore version
	const iser::IVersionInfo& versionInfo = archive.GetVersionInfo();
	quint32 serviceVersion;
	if (!versionInfo.GetVersionNumber(agentino::VI_SERVICE_MANAGER, serviceVersion)){
		serviceVersion = 0;
	}

	bool retVal = BaseClass::Serialize(archive);

	static iser::CArchiveTag pointTag("Point", "Point", iser::CArchiveTag::TT_GROUP);
	retVal = retVal && archive.BeginTag(pointTag);
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeQPointF(archive, m_point);
	retVal = retVal && archive.EndTag(pointTag);

//	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, m_propertyList, "PropertyList", "element");
//	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, m_linkList, "LinkList", "element");

	return retVal;
}



} // namespace imtauth


