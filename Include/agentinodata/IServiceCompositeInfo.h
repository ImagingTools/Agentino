#pragma once

// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{


class IServiceCompositeInfo: public istd::IChangeable
{
public:
	typedef QByteArray Id;
	typedef QList<Id> Ids;

	virtual QByteArray GetServiceId(const QUrl& url, const QString& connectionServiceTypeName) const = 0;
	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const = 0;
	virtual QString GetServiceStatus(const QByteArray& serviceId) const = 0;
	virtual QString GetDependantServiceStatus(const QByteArray& serviceId) const = 0;
	virtual Ids GetDependencyServices(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata
