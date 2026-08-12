// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QMap>

// Agentino includes
#include <agentinodata/IServiceCompositeInfo.h>


namespace agentinotest
{

/**
	Mock implementation of IServiceCompositeInfo for testing topology and service status controllers.
*/
class CMockServiceCompositeInfo: virtual public agentinodata::IServiceCompositeInfo
{
public:
	CMockServiceCompositeInfo() = default;

	void SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status)
	{
		m_serviceStatuses[serviceId] = status;
	}

	void SetStateOfRequiredServices(const QByteArray& serviceId, StateOfRequiredServices state)
	{
		m_requiredStates[serviceId] = state;
	}

	void SetServiceIdForConnection(const QByteArray& connectionId, const QByteArray& serviceId)
	{
		m_connectionToService[connectionId] = serviceId;
	}

	void SetServiceName(const QByteArray& serviceId, const QString& name)
	{
		m_serviceNames[serviceId] = name;
	}

	void SetServiceAgentName(const QByteArray& serviceId, const QString& agentName)
	{
		m_serviceAgentNames[serviceId] = agentName;
	}

	void SetDependencyServices(const QByteArray& serviceId, const Ids& deps)
	{
		m_dependencies[serviceId] = deps;
	}

	// reimplemented (agentinodata::IServiceCompositeInfo)
	virtual QByteArray GetServiceId(const QUrl& /*url*/) const override
	{
		return QByteArray();
	}

	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const override
	{
		return m_connectionToService.value(dependantServiceConnectionId);
	}

	virtual agentinodata::IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override
	{
		return m_serviceStatuses.value(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
	}

	virtual StateOfRequiredServices GetStateOfRequiredServices(const QByteArray& serviceId) const override
	{
		return m_requiredStates.value(serviceId, SORS_UNDEFINED);
	}

	virtual Ids GetDependencyServices(const QByteArray& serviceId) const override
	{
		return m_dependencies.value(serviceId);
	}

	virtual QString GetServiceName(const QByteArray& serviceId) const override
	{
		return m_serviceNames.value(serviceId);
	}

	virtual QString GetServiceAgentName(const QByteArray& serviceId) const override
	{
		return m_serviceAgentNames.value(serviceId);
	}

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override { return 0; }
	virtual bool CopyFrom(const IChangeable& /*object*/, CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return false; }
	virtual bool ResetData(CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return true; }

	void Reset()
	{
		m_serviceStatuses.clear();
		m_requiredStates.clear();
		m_connectionToService.clear();
		m_serviceNames.clear();
		m_serviceAgentNames.clear();
		m_dependencies.clear();
	}

private:
	QMap<QByteArray, agentinodata::IServiceStatusInfo::ServiceStatus> m_serviceStatuses;
	QMap<QByteArray, StateOfRequiredServices> m_requiredStates;
	QMap<QByteArray, QByteArray> m_connectionToService;
	QMap<QByteArray, QString> m_serviceNames;
	QMap<QByteArray, QString> m_serviceAgentNames;
	QMap<QByteArray, Ids> m_dependencies;
};

} // namespace agentinotest
