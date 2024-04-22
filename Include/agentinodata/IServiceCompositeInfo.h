#pragma once

// ACF includes
#include <istd/IChangeable.h>
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


class IServiceCompositeInfo: public istd::IChangeable
{
public:
	typedef QByteArray Id;
	typedef QList<Id> Ids;
	enum StateOfRequiredServices{
		SORS_UNDEFINED,
		SORS_RUNNING,
		SORS_NOT_RUNNING
	};

	I_DECLARE_ENUM(StateOfRequiredServices, SORS_RUNNING, SORS_NOT_RUNNING);

	virtual QByteArray GetServiceId(const QUrl& url, const QString& connectionServiceTypeName) const = 0;
	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const = 0;
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const = 0;
	virtual StateOfRequiredServices GetStateOfRequiredServices(const QByteArray& serviceId) const = 0;
	virtual Ids GetDependencyServices(const QByteArray& serviceId) const = 0;
	virtual QString GetServiceName(const QByteArray& serviceId) const = 0;
	virtual QString GetServiceAgentName(const QByteArray& serviceId) const = 0;
};

enum StateOfRequiredServices2{
	SORS_UNDEFINED,
	SORS_RUNNING,
	SORS_NOT_RUNNING
};

// class EnumRepresentation
// {
I_DECLARE_ENUM(StateOfRequiredServices2, SORS_RUNNING, SORS_NOT_RUNNING);
// };

} // namespace agentinodata
