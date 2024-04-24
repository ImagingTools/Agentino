#pragma once


// Qt includes
#include <QtCore/QProcess>

// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{

/**
	Interface for providing service status.
	\ingroup Service
*/
class IServiceStatusProvider: virtual public istd::IChangeable
{
public:
	struct NotifierStatusInfo
	{
		QByteArray serviceId;
		IServiceStatusInfo::ServiceStatus serviceStatus;
	};

	static const QByteArray CN_STATUS_CHANGED;

	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


Q_DECLARE_METATYPE(agentinodata::IServiceStatusProvider::NotifierStatusInfo)

