// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentinotest
{

/**
	Mock/extended CAgentInfo for testing.
	Provides a concrete agent info instance with test data.
*/
class CMockAgentInfo: public agentinodata::CAgentInfo
{
public:
	CMockAgentInfo(
		const QString& computerName = "TestComputer",
		const QString& version = "2.0.0")
	{
		SetComputerName(computerName);
		SetVersion(version);
		SetLastConnection(QDateTime::currentDateTime());
	}

	void AddService(const QByteArray& serviceId, agentinodata::CServiceInfo* serviceInfoPtr)
	{
		m_serviceCollection.InsertNewObject("Service", serviceInfoPtr->GetServiceName(), serviceInfoPtr->GetServiceDescription(), serviceInfoPtr, serviceId);
	}
};

} // namespace agentinotest
