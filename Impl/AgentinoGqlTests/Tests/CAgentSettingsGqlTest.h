// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtTest/QtTest>
#include <QtCore/QJsonObject>

// Test includes
#include "TestHelpers.h"

// Agentino GQL includes
#include <agentgql/CAgentSettingsControllerComp.h>


namespace agentinotest
{


/**
	Test suite for Agent Settings GQL queries:
	- CAgentSettingsControllerComp creates internal GQL response
	  containing agent connection settings
*/
class CAgentSettingsGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Response structure tests
	void testCreateInternalResponse_Structure();
	void testCreateInternalResponse_EmptyRequest();

	// Agent settings field tests
	void testAgentSettings_ConnectionInterface();
	void testAgentSettings_LoginSettings();

	// JSON response validation
	void testJsonResponse_ContainsData();
	void testJsonResponse_NoErrors();
};


} // namespace agentinotest
