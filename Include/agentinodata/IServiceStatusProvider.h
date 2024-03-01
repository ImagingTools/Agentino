#pragma once


// Qt includes
#include <QtCore/QProcess>

// ACF includes
#include <istd/IChangeable.h>


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
		QProcess::ProcessState serviceStatus;
	};

	static const QByteArray CN_STATUS_CHANGED;

	virtual QProcess::ProcessState GetServiceStatus(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


Q_DECLARE_METATYPE(agentinodata::IServiceStatusProvider::NotifierStatusInfo)

