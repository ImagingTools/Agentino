// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtTest/QtTest>
#include <QtCore/QJsonObject>

// Test includes
#include "TestHelpers.h"

// Mock includes
#include "../Mocks/CMockServiceController.h"
#include "../Mocks/CMockObjectCollection.h"
#include "../Mocks/CMockServiceCompositeInfo.h"
#include "../Mocks/CMockServiceManager.h"

// Agentino GQL includes
#include <agentinogql/CServiceControllerProxyComp.h>
#include <agentinogql/CServerServiceCollectionControllerComp.h>

// SDL includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinotest
{


/**
	Test suite for Server-side Service GQL queries (the proxy layer):
	- ServiceControllerProxy (Start/Stop/GetStatus/UpdateConnectionUrl/LoadPlugin)
	- ServerServiceCollectionController (ListObjects/SetupGqlItem/GetElementMetaInfo)
*/
class CServerServiceGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// ServiceControllerProxy tests
	void testProxy_StartService_SetsStatusViaManager();
	void testProxy_StopService_SetsStatusViaManager();
	void testProxy_GetServiceStatus_ReturnsCorrectStatus();
	void testProxy_GetServiceStatus_AllStatusValues();

	// ServerServiceCollection tests
	void testServerCollection_EmptyList();
	void testServerCollection_ServiceItemFields();
	void testServerCollection_MultipleServices_StatusMapping();
	void testServerCollection_DependencyStatusInfo();
	void testServerCollection_CompositeServiceInfo();

	// ServiceManager integration tests
	void testServiceManager_AddService();
	void testServiceManager_RemoveService();
	void testServiceManager_SetService();
	void testServiceManager_ServiceExists();

	// SDL response structure tests
	void testSdlServiceItem_AllFields();
	void testSdlServiceData_WithConnections();
	void testSdlServiceListPayload_Structure();
	void testSdlUpdateConnectionUrlResponse_Success();
	void testSdlUpdateConnectionUrlResponse_Failure();
	void testSdlPluginInfo_Structure();

private:
	CMockServiceController m_serviceController;
	CMockServiceManager m_serviceManager;
	CMockObjectCollection m_serviceCollection;
	CMockObjectCollection m_serviceStatusCollection;
	CMockObjectCollection m_agentCollection;
	CMockServiceCompositeInfo m_serviceCompositeInfo;
};


} // namespace agentinotest
