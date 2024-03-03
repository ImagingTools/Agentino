#pragma once


// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


class IServiceManager: virtual public istd::IChangeable
{
public:
	/**
		Change notification flags.
	*/
	enum ChangeFlags
	{
		/**
			Service was added.
		*/
		CF_SERVICE_ADDED = 20000,

		/**
			Service was changed.
		*/
		CF_SERVICE_UPDATED,

		/**
			Service was removed.
		*/
		CF_SERVICE_REMOVED
	};

	virtual bool AddService(
				const QByteArray& agentId,
				const IServiceInfo& serviceInfo,
				const QByteArray& serviceId = QByteArray(),
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString()) = 0;
	virtual bool RemoveService(const QByteArray& agentId, const QByteArray& serviceId) = 0;
	virtual bool SetService(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				const IServiceInfo& serviceInfo,
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString()) = 0;
	virtual bool ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


