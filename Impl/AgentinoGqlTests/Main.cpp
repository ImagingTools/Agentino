// SPDX-License-Identifier: LicenseRef-Agentino-Commercial

// Qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtTest/QtTest>

// Test suites
#include "Tests/CAgentsGqlTest.h"
#include "Tests/CServicesGqlTest.h"
#include "Tests/CTopologyGqlTest.h"
#include "Tests/CServiceLogGqlTest.h"
#include "Tests/CServerServiceGqlTest.h"
#include "Tests/CAgentSettingsGqlTest.h"


/**
	Main entry point for Agentino GQL Tests executable.
	Runs all GraphQL controller test suites using QTest framework.

	Tests cover all GQL queries defined in SDL schemas:
	- Agents.sdl: AgentsList, GetAgent, UpdateAgent, AddAgent
	- Services.sdl: ServicesList, GetService, AddService, UpdateService,
	                StartService, StopService, GetServiceStatus,
	                ServicesRemove, LoadPlugin, UpdateConnectionUrl
	- Topology.sdl: GetTopology, SaveTopology
	- ServiceLog.sdl: GetServiceLog

	Usage:
		AgentinoGqlTests                        - Run all tests
		AgentinoGqlTests -v2                    - Run all tests with verbose output
		AgentinoGqlTests CAgentsGqlTest         - Run only agent tests
		AgentinoGqlTests -functions             - List all test functions
*/
int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	app.setApplicationName("AgentinoGqlTests");

	qInfo() << "========================================";
	qInfo() << "  Agentino GQL Controller Tests";
	qInfo() << "========================================";
	qInfo() << "";

	int status = 0;

	// Run Agents GQL tests
	{
		agentinotest::CAgentsGqlTest agentsTest;
		status |= QTest::qExec(&agentsTest, argc, argv);
	}

	// Run Services GQL tests
	{
		agentinotest::CServicesGqlTest servicesTest;
		status |= QTest::qExec(&servicesTest, argc, argv);
	}

	// Run Topology GQL tests
	{
		agentinotest::CTopologyGqlTest topologyTest;
		status |= QTest::qExec(&topologyTest, argc, argv);
	}

	// Run ServiceLog GQL tests
	{
		agentinotest::CServiceLogGqlTest serviceLogTest;
		status |= QTest::qExec(&serviceLogTest, argc, argv);
	}

	// Run Server Service GQL tests
	{
		agentinotest::CServerServiceGqlTest serverServiceTest;
		status |= QTest::qExec(&serverServiceTest, argc, argv);
	}

	// Run Agent Settings GQL tests
	{
		agentinotest::CAgentSettingsGqlTest agentSettingsTest;
		status |= QTest::qExec(&agentSettingsTest, argc, argv);
	}

	qInfo() << "";
	qInfo() << "========================================";
	if (status == 0){
		qInfo() << "  All test suites PASSED";
	}
	else{
		qWarning() << "  Some test suites FAILED";
	}
	qInfo() << "========================================";

	return status;
}
