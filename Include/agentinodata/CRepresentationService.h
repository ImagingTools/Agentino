#pragma once


// ImtCore includes
#include <agentinodata/IRepresentationService.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


class CRepresentationService: public CServiceInfo, virtual public IRepresentationService
{

public:
	typedef CServiceInfo BaseClass;
	CRepresentationService(ServiceType serviceType = ST_ACF);

	// reimplemented (agentinodata::IRepresentationService)
	virtual QPointF GetPoint() const override;
	virtual void SetPoint(const QPointF& point) override;
	virtual PropertyList GetExternPropertyList() const override;
	virtual void SetExternPropertyList(const PropertyList& propertyList) override;
	virtual LinkList GetLinkList() const override;
	virtual void SetLinkList(const LinkList& linkList) override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

protected:
	QPointF m_point;
	PropertyList m_propertyList;
	LinkList m_linkList;
};


typedef imtbase::TIdentifiableWrap<CRepresentationService> CIdentifiableRepresentationService;



} // namespace imtauth



