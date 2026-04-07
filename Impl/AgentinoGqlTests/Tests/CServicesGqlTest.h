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
#include "../Mocks/CMockServiceInfo.h"
#include "../Mocks/CMockConnectionCollectionProvider.h"

// Agentino GQL includes
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CServiceCollectionControllerComp.h>

// SDL includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinotest
{


/**
	Test suite for Services GQL queries:
	- ServicesList
	- GetService
	- AddService / UpdateService
	- StartService / StopService
	- GetServiceStatus
	- ServicesRemove
	- LoadPlugin
	- UpdateConnectionUrl
*/
class CServicesGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Collection tests
	void testServicesList_EmptyCollection();
	void testServicesList_SingleService();
	void testServicesList_MultipleServices();
	void testServicesList_WithPagination();

	// Get service tests
	void testGetService_ValidId();
	void testGetService_InvalidId();
	void testGetService_ServiceFields();

	// Add/Update service tests
	void testAddService_ValidData();
	void testAddService_MinimalData();
	void testUpdateService_ValidData();
	void testUpdateService_NonExistentId();

	// Service lifecycle tests
	void testStartService_ValidId();
	void testStartService_EmptyId();
	void testStartService_InvalidVersion();
	void testStopService_ValidId();
	void testStopService_EmptyId();
	void testGetServiceStatus_Running();
	void testGetServiceStatus_NotRunning();
	void testGetServiceStatus_Undefined();
	void testGetServiceStatus_EmptyId();

	// Remove service tests
	void testServicesRemove_SingleService();
	void testServicesRemove_MultipleServices();

	// Plugin and connection tests
	void testLoadPlugin_ValidPath();
	void testLoadPlugin_EmptyPath();
	void testLoadPlugin_InvalidVersion();
	void testUpdateConnectionUrl_ValidData();
	void testUpdateConnectionUrl_InvalidServiceId();

	// Status enum mapping tests
	void testServiceStatusEnum_AllValues();

private:
	CMockServiceController m_serviceController;
	CMockObjectCollection m_serviceCollection;
	CMockConnectionCollectionProvider m_connectionProvider;
	QList<CMockServiceInfo*> m_testServices;
};


} // namespace agentinotest
