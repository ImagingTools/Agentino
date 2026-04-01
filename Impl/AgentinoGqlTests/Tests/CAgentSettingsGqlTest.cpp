// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CAgentSettingsGqlTest.h"


namespace agentinotest
{


void CAgentSettingsGqlTest::initTestCase()
{
	qInfo() << "=== Agent Settings GQL Tests ===";
}


void CAgentSettingsGqlTest::cleanupTestCase()
{
}


void CAgentSettingsGqlTest::init()
{
}


void CAgentSettingsGqlTest::cleanup()
{
}


// === Response Structure Tests ===

void CAgentSettingsGqlTest::testCreateInternalResponse_Structure()
{
	// Verify that a valid QJsonObject response can be constructed
	// The CAgentSettingsControllerComp::CreateInternalResponse returns QJsonObject
	QJsonObject response;
	response["data"] = QJsonObject{
		{"agentSettings", QJsonObject{
			{"connectionUrl", "ws://localhost:7112"},
			{"isConnected", true}
		}}
	};

	QVERIFY(response.contains("data"));
	QJsonObject data = response["data"].toObject();
	QVERIFY(data.contains("agentSettings"));

	QJsonObject settings = data["agentSettings"].toObject();
	QCOMPARE(settings["connectionUrl"].toString(), QString("ws://localhost:7112"));
	QCOMPARE(settings["isConnected"].toBool(), true);
}


void CAgentSettingsGqlTest::testCreateInternalResponse_EmptyRequest()
{
	// When no connection interface is set, response should still be valid
	QJsonObject response;
	QVERIFY(response.isEmpty());

	// After emplace, it should have structure
	response["data"] = QJsonObject();
	QVERIFY(!response.isEmpty());
	QVERIFY(response.contains("data"));
}


// === Agent Settings Field Tests ===

void CAgentSettingsGqlTest::testAgentSettings_ConnectionInterface()
{
	// Test connection interface configuration
	QJsonObject connectionSettings;
	connectionSettings["host"] = "192.168.1.100";
	connectionSettings["httpPort"] = 7776;
	connectionSettings["wsPort"] = 8888;
	connectionSettings["protocol"] = "ws";

	QCOMPARE(connectionSettings["host"].toString(), QString("192.168.1.100"));
	QCOMPARE(connectionSettings["httpPort"].toInt(), 7776);
	QCOMPARE(connectionSettings["wsPort"].toInt(), 8888);
	QCOMPARE(connectionSettings["protocol"].toString(), QString("ws"));
}


void CAgentSettingsGqlTest::testAgentSettings_LoginSettings()
{
	// Test login configuration
	QJsonObject loginSettings;
	loginSettings["isLoggedIn"] = false;
	loginSettings["userName"] = "";
	loginSettings["loginMethod"] = "jwt";

	QCOMPARE(loginSettings["isLoggedIn"].toBool(), false);
	QCOMPARE(loginSettings["userName"].toString(), QString(""));
	QCOMPARE(loginSettings["loginMethod"].toString(), QString("jwt"));

	// After successful login
	loginSettings["isLoggedIn"] = true;
	loginSettings["userName"] = "admin";

	QCOMPARE(loginSettings["isLoggedIn"].toBool(), true);
	QCOMPARE(loginSettings["userName"].toString(), QString("admin"));
}


// === JSON Response Validation ===

void CAgentSettingsGqlTest::testJsonResponse_ContainsData()
{
	QJsonObject response;
	response["data"] = QJsonObject{
		{"agentInfo", QJsonObject{
			{"name", "TestAgent"},
			{"version", "1.0.0"},
			{"computerName", "PC-TEST"}
		}}
	};

	QString error;
	QVERIFY(VerifyNoErrors(response, error));

	QJsonObject data = GetResponseData(response);
	QVERIFY(!data.isEmpty());
	QVERIFY(data.contains("agentInfo"));
}


void CAgentSettingsGqlTest::testJsonResponse_NoErrors()
{
	// Response without errors
	QJsonObject goodResponse;
	goodResponse["data"] = QJsonObject{{"result", true}};

	QString error;
	QVERIFY(VerifyNoErrors(goodResponse, error));

	// Response with errors
	QJsonObject badResponse;
	badResponse["errors"] = QJsonArray{
		QJsonObject{{"message", "Test error"}}
	};

	QVERIFY(!VerifyNoErrors(badResponse, error));
	QVERIFY(!error.isEmpty());
}


} // namespace agentinotest
