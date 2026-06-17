// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentinotest
{

/**
	Mock/extended CServiceInfo for testing.
	Provides a concrete service info instance with test data.
*/
class CMockServiceInfo: public agentinodata::CServiceInfo
{
public:
	CMockServiceInfo(
		const QString& name = "TestService",
		const QString& description = "Test service description",
		const QByteArray& path = "/opt/test/service")
		: CServiceInfo("TestType", ST_PLUGIN)
	{
		SetServiceName(name);
		SetServiceDescription(description);
		SetServicePath(path);
		SetServiceVersion("1.0.0");
		SetServiceTypeId("TestType");
		SetIsAutoStart(false);
	}
};

} // namespace agentinotest
