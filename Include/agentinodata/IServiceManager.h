// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
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
	virtual bool RemoveServices(
				const QByteArray& agentId,
				const imtbase::ICollectionInfo::Ids& serviceIds) = 0;
	virtual bool SetService(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				const IServiceInfo& serviceInfo,
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString(),
				bool beQuiet = false) = 0;
	virtual bool ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const = 0;
	virtual IServiceInfo* GetService(const QByteArray& agentId, const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


