#pragma once

// Qt include
#include <QtCore/QPoint>
#include <QtCore/QUuid>

// Acf includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


/**
	Interface for describing an RepresentationService.
	\ingroup Service
*/
class IRepresentationService:
		virtual public IServiceInfo
{
public:
	class ExternProperty: virtual public iser::ISerializable
	{
		ExternProperty(const QByteArray& id = QByteArray());

		// reimplemented (iser::ISerializable)
		virtual bool Serialize(iser::IArchive &archive) override;

		QByteArray id;
		QByteArray name;
		QByteArray description;
		QByteArray value;
	};

	typedef QList<ExternProperty> PropertyList;

	class Link: virtual public iser::ISerializable
	{
	public:
		Link(const QByteArray& id = QByteArray());

		// reimplemented (iser::ISerializable)
		virtual bool Serialize(iser::IArchive &archive) override;

		QByteArray id;
		QByteArray name;
		QByteArray description;
		QByteArray agentId;
		QByteArray serviceId;
		QByteArray propertyId;
		QByteArray serviceTypeId;
	};

	typedef QList<Link> LinkList;

	virtual QPointF GetPoint() const = 0;
	virtual void SetPoint(const QPointF& point) = 0;
	virtual PropertyList GetExternPropertyList() const = 0;
	virtual void SetExternPropertyList(const PropertyList& propertyList) = 0;
	virtual LinkList GetLinkList() const = 0;
	virtual void SetLinkList(const LinkList& linkList) = 0;
};


} // namespace agentinodata


