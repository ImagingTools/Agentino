// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QMap>

// Agentino includes
#include <agentinodata/IServiceController.h>


namespace agentinotest
{

/**
	Mock implementation of IServiceController for testing GQL controllers.
*/
class CMockServiceController: virtual public agentinodata::IServiceController
{
public:
	CMockServiceController() = default;

	void SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status)
	{
		m_serviceStatuses[serviceId] = status;
	}

	// reimplemented (agentinodata::IServiceController)
	virtual bool StartService(const QByteArray& serviceId) override
	{
		m_serviceStatuses[serviceId] = agentinodata::IServiceStatusInfo::SS_RUNNING;
		m_startCalls << serviceId;
		return true;
	}

	virtual bool StopService(const QByteArray& serviceId) override
	{
		m_serviceStatuses[serviceId] = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
		m_stopCalls << serviceId;
		return true;
	}

	// reimplemented (agentinodata::IServiceStatusProvider)
	virtual agentinodata::IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override
	{
		if (m_serviceStatuses.contains(serviceId)){
			return m_serviceStatuses[serviceId];
		}
		return agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	}

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override { return 0; }
	virtual bool CopyFrom(const IChangeable& /*object*/, CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return false; }
	virtual bool ResetData(CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return true; }

	// Test accessors
	QList<QByteArray> GetStartCalls() const { return m_startCalls; }
	QList<QByteArray> GetStopCalls() const { return m_stopCalls; }
	void Reset()
	{
		m_serviceStatuses.clear();
		m_startCalls.clear();
		m_stopCalls.clear();
	}

private:
	QMap<QByteArray, agentinodata::IServiceStatusInfo::ServiceStatus> m_serviceStatuses;
	QList<QByteArray> m_startCalls;
	QList<QByteArray> m_stopCalls;
};

} // namespace agentinotest
