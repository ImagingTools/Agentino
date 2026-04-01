// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QMap>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinotest
{

/**
	Mock implementation of IServiceManager for testing service proxy controllers.
*/
class CMockServiceManager: virtual public agentinodata::IServiceManager
{
public:
	CMockServiceManager() = default;

	// reimplemented (agentinodata::IServiceManager)
	virtual bool AddService(
				const QByteArray& agentId,
				const agentinodata::IServiceInfo& serviceInfo,
				const QByteArray& serviceId = QByteArray(),
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString()) override
	{
		ServiceEntry entry;
		entry.agentId = agentId;
		entry.name = serviceName.isEmpty() ? serviceInfo.GetServiceName() : serviceName;
		entry.description = serviceDescription.isEmpty() ? serviceInfo.GetServiceDescription() : serviceDescription;
		entry.path = serviceInfo.GetServicePath();

		QByteArray id = serviceId.isEmpty() ? QByteArray::number(m_nextId++) : serviceId;
		m_services[id] = entry;
		m_addCalls++;
		return true;
	}

	virtual bool RemoveServices(
				const QByteArray& /*agentId*/,
				const imtbase::ICollectionInfo::Ids& serviceIds) override
	{
		for (const auto& id: serviceIds){
			m_services.remove(id);
		}
		m_removeCalls++;
		return true;
	}

	virtual bool SetService(
				const QByteArray& /*agentId*/,
				const QByteArray& serviceId,
				const agentinodata::IServiceInfo& serviceInfo,
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString(),
				bool /*beQuiet*/ = false) override
	{
		if (!m_services.contains(serviceId)){
			return false;
		}
		ServiceEntry& entry = m_services[serviceId];
		entry.name = serviceName.isEmpty() ? serviceInfo.GetServiceName() : serviceName;
		entry.description = serviceDescription.isEmpty() ? serviceInfo.GetServiceDescription() : serviceDescription;
		entry.path = serviceInfo.GetServicePath();
		m_setCalls++;
		return true;
	}

	virtual bool ServiceExists(const QByteArray& /*agentId*/, const QByteArray& serviceId) const override
	{
		return m_services.contains(serviceId);
	}

	virtual agentinodata::IServiceInfo* GetService(const QByteArray& /*agentId*/, const QByteArray& serviceId) const override
	{
		Q_UNUSED(serviceId)
		return nullptr;
	}

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override { return 0; }
	virtual bool CopyFrom(const IChangeable& /*object*/, CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return false; }
	virtual bool ResetData(CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override
	{
		m_services.clear();
		return true;
	}

	// Test accessors
	int GetAddCallCount() const { return m_addCalls; }
	int GetRemoveCallCount() const { return m_removeCalls; }
	int GetSetCallCount() const { return m_setCalls; }
	int GetServiceCount() const { return m_services.count(); }

	void Reset()
	{
		m_services.clear();
		m_addCalls = 0;
		m_removeCalls = 0;
		m_setCalls = 0;
	}

private:
	struct ServiceEntry
	{
		QByteArray agentId;
		QString name;
		QString description;
		QByteArray path;
	};

	QMap<QByteArray, ServiceEntry> m_services;
	int m_nextId = 1;
	int m_addCalls = 0;
	int m_removeCalls = 0;
	int m_setCalls = 0;
};

} // namespace agentinotest
