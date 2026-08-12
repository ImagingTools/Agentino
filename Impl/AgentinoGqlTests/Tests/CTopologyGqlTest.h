// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

// Test includes
#include "TestHelpers.h"

// Mock includes
#include "../Mocks/CMockServiceController.h"
#include "../Mocks/CMockObjectCollection.h"
#include "../Mocks/CMockServiceCompositeInfo.h"
#include "../Mocks/CMockAgentInfo.h"
#include "../Mocks/CMockServiceInfo.h"

// Agentino GQL includes
#include <agentinogql/CTopologyControllerComp.h>
#include <agentinogql/CAgentCollectionControllerComp.h>

// SDL includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Topology.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents.h>


namespace agentinotest
{


/**
	Test suite for Topology GQL queries:
	- GetTopology
	- SaveTopology
*/
class CTopologyGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Query tests
	void testGetTopology_EmptyCollection();
	void testGetTopology_SingleAgent_SingleService();
	void testGetTopology_MultipleAgents_MultipleServices();
	void testGetTopology_ServiceStatusRunning();
	void testGetTopology_ServiceStatusNotRunning();
	void testGetTopology_ServiceStatusUndefined();
	void testGetTopology_ServiceWithDependencyLinks();
	void testGetTopology_ServiceIcons_Running();
	void testGetTopology_ServiceIcons_Stopped();
	void testGetTopology_ServiceIcons_Alert();
	void testGetTopology_DependencyWarningIcons();

	// Mutation tests
	void testSaveTopology_EmptyServiceList();
	void testSaveTopology_SingleService();
	void testSaveTopology_MultipleServices();
	void testSaveTopology_InvalidVersion();
	void testSaveTopology_NullTopologyCollection();

private:
	void SetupTestAgent(const QByteArray& agentId, const QString& agentName,
						const QByteArray& serviceId, const QString& serviceName);

	CMockObjectCollection m_agentCollection;
	CMockObjectCollection m_topologyCollection;
	CMockServiceCompositeInfo m_serviceCompositeInfo;

	// Owned test data objects
	QList<CMockAgentInfo*> m_testAgents;
	QList<CMockServiceInfo*> m_testServices;
};


} // namespace agentinotest
