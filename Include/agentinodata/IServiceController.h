#pragma once


// Qt includes
#include <QtCore/QProcess>

// ACF includes
#include <istd/IChangeable.h>

#undef StartService

namespace agentinodata
{


/**
	Interface for describing an service.
	\ingroup Service
*/
class IServiceController: virtual public istd::IChangeable
{
public:
	struct NotifierStatusInfo
	{
		QByteArray serviceId;
		QProcess::ProcessState serviceStatus;
	};

	static const QByteArray CN_STATUS_CHANGED;

	virtual QProcess::ProcessState GetServiceStatus(const QByteArray& serviceId) const = 0;
	virtual bool StartService(const QByteArray& serviceId) = 0;
	virtual bool StopService(const QByteArray& serviceId) = 0;
};


} // namespace agentinodata


Q_DECLARE_METATYPE(agentinodata::IServiceController::NotifierStatusInfo)


