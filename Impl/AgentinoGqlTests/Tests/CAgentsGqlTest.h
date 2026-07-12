// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>

// Test includes
#include "TestHelpers.h"

// Mock includes
#include "../Mocks/CMockObjectCollection.h"
#include "../Mocks/CMockAgentInfo.h"
#include "../Mocks/CMockServiceInfo.h"

// Agentino GQL includes
#include <agentinogql/CAgentCollectionControllerComp.h>

// SDL includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents.h>


namespace agentinotest
{


/**
	Test suite for Agents GQL queries:
	- AgentsList
	- GetAgent
	- UpdateAgent
	- AddAgent
*/
class CAgentsGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Collection tests
	void testAgentsList_EmptyCollection();
	void testAgentsList_SingleAgent();
	void testAgentsList_MultipleAgents();
	void testAgentsList_AgentWithServices();

	// Get agent tests
	void testGetAgent_ValidId();
	void testGetAgent_InvalidId();
	void testGetAgent_AllFields();
	void testGetAgent_TracingLevel();

	// Agent data representation tests
	void testAgentItem_Fields();
	void testAgentItem_StatusField();
	void testAgentItem_VersionField();
	void testAgentItem_LastConnectionField();
	void testAgentItem_ComputerNameField();
	void testAgentItem_ServicesField();

	// Mutation tests
	void testUpdateAgent_ValidData();
	void testUpdateAgent_NameOnly();
	void testUpdateAgent_DescriptionOnly();
	void testAddAgent_ValidData();
	void testAddAgent_WithComputerName();
	void testAddAgent_WithVersion();

	// SDL representation tests
	void testSdlAgentListPayload_Structure();
	void testSdlAgentData_Structure();
	void testSdlAgentDataInput_Structure();

private:
	CMockObjectCollection m_agentCollection;
	CMockObjectCollection m_agentStatusCollection;
	CMockObjectCollection m_serviceStatusCollection;
	QList<CMockAgentInfo*> m_testAgents;
	QList<CMockServiceInfo*> m_testServices;
};


} // namespace agentinotest
