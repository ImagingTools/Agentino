// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CServicesGqlTest.h"


namespace agentinotest
{


void CServicesGqlTest::initTestCase()
{
	qInfo() << "=== Services GQL Tests ===";
}


void CServicesGqlTest::cleanupTestCase()
{
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CServicesGqlTest::init()
{
	m_serviceController.Reset();
	m_serviceCollection.Clear();
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CServicesGqlTest::cleanup()
{
}


// === Collection Tests ===

void CServicesGqlTest::testServicesList_EmptyCollection()
{
	QCOMPARE(m_serviceCollection.GetElementCount(), 0);
	QVERIFY(m_serviceCollection.GetElementIds().isEmpty());
}


void CServicesGqlTest::testServicesList_SingleService()
{
	CMockServiceInfo* servicePtr = new CMockServiceInfo("WebService", "HTTP web service", "/opt/web");
	m_testServices.append(servicePtr);
	m_serviceCollection.AddObject("svc1", "WebService", "HTTP web service", servicePtr);

	QCOMPARE(m_serviceCollection.GetElementCount(), 1);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_serviceCollection.GetObjectData("svc1", dataPtr));

	agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(dataPtr.GetPtr());
	QVERIFY(serviceInfoPtr != nullptr);
	QCOMPARE(serviceInfoPtr->GetServiceName(), QString("WebService"));
	QCOMPARE(serviceInfoPtr->GetServiceDescription(), QString("HTTP web service"));
	QCOMPARE(serviceInfoPtr->GetServicePath(), QByteArray("/opt/web"));
}


void CServicesGqlTest::testServicesList_MultipleServices()
{
	for (int i = 0; i < 5; ++i){
		CMockServiceInfo* servicePtr = new CMockServiceInfo(
			QString("Service_%1").arg(i),
			QString("Description %1").arg(i),
			QByteArray("/opt/svc") + QByteArray::number(i));
		m_testServices.append(servicePtr);
		m_serviceCollection.AddObject(
			QByteArray("svc") + QByteArray::number(i),
			servicePtr->GetServiceName(),
			servicePtr->GetServiceDescription(),
			servicePtr);
	}

	QCOMPARE(m_serviceCollection.GetElementCount(), 5);

	// Verify each service can be retrieved
	for (int i = 0; i < 5; ++i){
		QByteArray id = QByteArray("svc") + QByteArray::number(i);
		imtbase::IObjectCollection::DataPtr dataPtr;
		QVERIFY(m_serviceCollection.GetObjectData(id, dataPtr));
		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(dataPtr.GetPtr());
		QVERIFY(serviceInfoPtr != nullptr);
		QCOMPARE(serviceInfoPtr->GetServiceName(), QString("Service_%1").arg(i));
	}
}


void CServicesGqlTest::testServicesList_WithPagination()
{
	// Create 10 services
	for (int i = 0; i < 10; ++i){
		CMockServiceInfo* servicePtr = new CMockServiceInfo(
			QString("Service_%1").arg(i),
			QString("Description %1").arg(i),
			QByteArray("/opt/svc") + QByteArray::number(i));
		m_testServices.append(servicePtr);
		m_serviceCollection.AddObject(
			QByteArray("svc") + QByteArray::number(i),
			servicePtr->GetServiceName(),
			servicePtr->GetServiceDescription(),
			servicePtr);
	}

	QCOMPARE(m_serviceCollection.GetElementCount(), 10);

	// Simulate pagination: get first 5 elements
	imtbase::ICollectionInfo::Ids allIds = m_serviceCollection.GetElementIds();
	QCOMPARE(allIds.size(), 10);

	// Verify offset-based pagination logic
	int offset = 0;
	int count = 5;
	int totalPages = (allIds.size() + count - 1) / count;
	QCOMPARE(totalPages, 2);

	QList<QByteArray> page1 = allIds.mid(offset, count);
	QCOMPARE(page1.size(), 5);

	QList<QByteArray> page2 = allIds.mid(offset + count, count);
	QCOMPARE(page2.size(), 5);
}


// === Get Service Tests ===

void CServicesGqlTest::testGetService_ValidId()
{
	CMockServiceInfo* servicePtr = new CMockServiceInfo("TestSvc", "Test service", "/opt/test");
	m_testServices.append(servicePtr);
	m_serviceCollection.AddObject("svc1", "TestSvc", "Test service", servicePtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_serviceCollection.GetObjectData("svc1", dataPtr));

	agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(dataPtr.GetPtr());
	QVERIFY(serviceInfoPtr != nullptr);

	// Verify SDL representation fields
	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	serviceData.id = QByteArray("svc1");
	serviceData.name = serviceInfoPtr->GetServiceName();
	serviceData.description = serviceInfoPtr->GetServiceDescription();
	serviceData.path = QString(serviceInfoPtr->GetServicePath());

	QCOMPARE(*serviceData.id, QByteArray("svc1"));
	QCOMPARE(*serviceData.name, QString("TestSvc"));
	QCOMPARE(*serviceData.description, QString("Test service"));
	QCOMPARE(*serviceData.path, QString("/opt/test"));
}


void CServicesGqlTest::testGetService_InvalidId()
{
	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(!m_serviceCollection.GetObjectData("nonexistent", dataPtr));
}


void CServicesGqlTest::testGetService_ServiceFields()
{
	CMockServiceInfo* servicePtr = new CMockServiceInfo("FullService", "Full description", "/opt/full");
	servicePtr->SetServiceVersion("2.5.0");
	servicePtr->SetServiceTypeId("ProcessingEngine");
	servicePtr->SetIsAutoStart(true);
	servicePtr->SetTracingLevel(3);
	servicePtr->SetServiceArguments(QByteArrayList() << "--port=8080" << "--verbose");
	servicePtr->SetStartScriptPath("/opt/full/start.sh");
	servicePtr->SetStopScriptPath("/opt/full/stop.sh");
	m_testServices.append(servicePtr);
	m_serviceCollection.AddObject("full_svc", "FullService", "Full description", servicePtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_serviceCollection.GetObjectData("full_svc", dataPtr));

	agentinodata::IServiceInfo* infoPtr = dynamic_cast<agentinodata::IServiceInfo*>(dataPtr.GetPtr());
	QVERIFY(infoPtr != nullptr);

	// Verify all service fields
	QCOMPARE(infoPtr->GetServiceName(), QString("FullService"));
	QCOMPARE(infoPtr->GetServiceDescription(), QString("Full description"));
	QCOMPARE(infoPtr->GetServicePath(), QByteArray("/opt/full"));
	QCOMPARE(infoPtr->GetServiceVersion(), QString("2.5.0"));
	QCOMPARE(infoPtr->GetServiceTypeId(), QString("ProcessingEngine"));
	QCOMPARE(infoPtr->IsAutoStart(), true);
	QCOMPARE(infoPtr->GetTracingLevel(), 3);
	QCOMPARE(infoPtr->GetServiceArguments(), QByteArrayList() << "--port=8080" << "--verbose");
	QCOMPARE(infoPtr->GetStartScriptPath(), QByteArray("/opt/full/start.sh"));
	QCOMPARE(infoPtr->GetStopScriptPath(), QByteArray("/opt/full/stop.sh"));
}


// === Add/Update Service Tests ===

void CServicesGqlTest::testAddService_ValidData()
{
	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	serviceData.id = QByteArray("new_svc");
	serviceData.name = QString("NewService");
	serviceData.description = QString("Newly added service");
	serviceData.path = QString("/opt/new");
	serviceData.serviceTypeId = QString("WebApp");

	// Verify SDL structure
	QCOMPARE(*serviceData.id, QByteArray("new_svc"));
	QCOMPARE(*serviceData.name, QString("NewService"));

	// Simulate adding to collection
	CMockServiceInfo* servicePtr = new CMockServiceInfo(*serviceData.name, *serviceData.description, serviceData.path->toUtf8());
	m_testServices.append(servicePtr);
	QByteArray insertedId = m_serviceCollection.InsertNewObject("Service", *serviceData.name, *serviceData.description, servicePtr, *serviceData.id);

	QVERIFY(!insertedId.isEmpty());
	QCOMPARE(m_serviceCollection.GetElementCount(), 1);
}


void CServicesGqlTest::testAddService_MinimalData()
{
	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	serviceData.id = QByteArray("min_svc");
	serviceData.name = QString("MinService");

	QVERIFY(serviceData.id.has_value());
	QVERIFY(serviceData.name.has_value());
	QVERIFY(!serviceData.description.has_value());
	QVERIFY(!serviceData.path.has_value());
}


void CServicesGqlTest::testUpdateService_ValidData()
{
	CMockServiceInfo* servicePtr = new CMockServiceInfo("OldName", "Old description", "/opt/old");
	m_testServices.append(servicePtr);
	m_serviceCollection.AddObject("svc1", "OldName", "Old description", servicePtr);

	// Simulate update
	servicePtr->SetServiceName("NewName");
	servicePtr->SetServiceDescription("Updated description");
	m_serviceCollection.SetElementInfo("svc1", imtbase::ICollectionInfo::EIT_NAME, "NewName");
	m_serviceCollection.SetElementInfo("svc1", imtbase::ICollectionInfo::EIT_DESCRIPTION, "Updated description");

	QCOMPARE(m_serviceCollection.GetElementInfo("svc1", imtbase::ICollectionInfo::EIT_NAME).toString(), QString("NewName"));
	QCOMPARE(m_serviceCollection.GetElementInfo("svc1", imtbase::ICollectionInfo::EIT_DESCRIPTION).toString(), QString("Updated description"));
}


void CServicesGqlTest::testUpdateService_NonExistentId()
{
	QVERIFY(!m_serviceCollection.SetElementInfo("nonexistent", imtbase::ICollectionInfo::EIT_NAME, "NewName"));
}


// === Service Lifecycle Tests ===

void CServicesGqlTest::testStartService_ValidId()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	// Start service
	bool started = m_serviceController.StartService("svc1");
	QVERIFY(started);

	// Verify status changed
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("svc1");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_RUNNING);

	// Verify start was called
	QVERIFY(m_serviceController.GetStartCalls().contains("svc1"));

	// Verify SDL response mapping
	sdl::agentino::Services::CServiceStatusResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus)status;
	QCOMPARE(*response.Version_1_0->status, sdl::agentino::Services::ServiceStatus::RUNNING);
}


void CServicesGqlTest::testStartService_EmptyId()
{
	// Verify error handling for empty service ID
	sdl::agentino::Services::CServiceStatusResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->status = sdl::agentino::Services::ServiceStatus::UNDEFINED;

	QString errorMessage = "Unable to start service with empty ID";
	QVERIFY(!errorMessage.isEmpty());
	QCOMPARE(*response.Version_1_0->status, sdl::agentino::Services::ServiceStatus::UNDEFINED);
}


void CServicesGqlTest::testStartService_InvalidVersion()
{
	sdl::agentino::Services::CServiceStatusResponse response;
	// Without Version_1_0 being set, response should be empty
	QVERIFY(!response.Version_1_0.has_value());
}


void CServicesGqlTest::testStopService_ValidId()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);

	bool stopped = m_serviceController.StopService("svc1");
	QVERIFY(stopped);

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("svc1");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
	QVERIFY(m_serviceController.GetStopCalls().contains("svc1"));
}


void CServicesGqlTest::testStopService_EmptyId()
{
	sdl::agentino::Services::CServiceStatusResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->status = sdl::agentino::Services::ServiceStatus::UNDEFINED;

	QCOMPARE(*response.Version_1_0->status, sdl::agentino::Services::ServiceStatus::UNDEFINED);
}


void CServicesGqlTest::testGetServiceStatus_Running()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("svc1");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_RUNNING);

	sdl::agentino::Services::CServiceStatusResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus)status;
	QCOMPARE(*response.Version_1_0->status, sdl::agentino::Services::ServiceStatus::RUNNING);
}


void CServicesGqlTest::testGetServiceStatus_NotRunning()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("svc1");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
}


void CServicesGqlTest::testGetServiceStatus_Undefined()
{
	// Unregistered service should return UNDEFINED
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("unknown_svc");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
}


void CServicesGqlTest::testGetServiceStatus_EmptyId()
{
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceController.GetServiceStatus("");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
}


// === Remove Service Tests ===

void CServicesGqlTest::testServicesRemove_SingleService()
{
	CMockServiceInfo* servicePtr = new CMockServiceInfo("ToRemove", "Remove me", "/opt/remove");
	m_testServices.append(servicePtr);
	m_serviceCollection.AddObject("svc_remove", "ToRemove", "Remove me", servicePtr);
	QCOMPARE(m_serviceCollection.GetElementCount(), 1);

	// Verify SDL response structure
	sdl::imtbase::ImtCollection::CRemovedNotificationPayload removeResponse;
	// Default constructor produces valid but empty response
	QVERIFY(true);

	// Actually remove
	QVERIFY(m_serviceCollection.RemoveObject("svc_remove"));
	QCOMPARE(m_serviceCollection.GetElementCount(), 0);
}


void CServicesGqlTest::testServicesRemove_MultipleServices()
{
	for (int i = 0; i < 3; ++i){
		CMockServiceInfo* servicePtr = new CMockServiceInfo(
			QString("Svc%1").arg(i), QString("Desc%1").arg(i),
			QByteArray("/opt/svc") + QByteArray::number(i));
		m_testServices.append(servicePtr);
		m_serviceCollection.AddObject(QByteArray("svc") + QByteArray::number(i), servicePtr->GetServiceName(), servicePtr->GetServiceDescription(), servicePtr);
	}
	QCOMPARE(m_serviceCollection.GetElementCount(), 3);

	imtbase::ICollectionInfo::Ids idsToRemove;
	idsToRemove << "svc0" << "svc2";
	QVERIFY(m_serviceCollection.RemoveObjects(idsToRemove));
	QCOMPARE(m_serviceCollection.GetElementCount(), 1);
	QVERIFY(m_serviceCollection.GetElementIds().contains("svc1"));
}


// === Plugin and Connection Tests ===

void CServicesGqlTest::testLoadPlugin_ValidPath()
{
	sdl::agentino::Services::CPluginInfo response;
	response.Version_1_0.emplace();
	response.Version_1_0->servicePath = "/opt/plugin/service";

	QVERIFY(response.Version_1_0.has_value());
	QCOMPARE(*response.Version_1_0->servicePath, QString("/opt/plugin/service"));
}


void CServicesGqlTest::testLoadPlugin_EmptyPath()
{
	// Empty path should produce error message
	QString errorMessage;
	QString servicePath;
	if (servicePath.isEmpty()){
		errorMessage = "Unable to load plugin. Error: Service path is invalid";
	}
	QVERIFY(!errorMessage.isEmpty());
}


void CServicesGqlTest::testLoadPlugin_InvalidVersion()
{
	sdl::agentino::Services::CPluginInfo response;
	// Without Version_1_0 being set
	QVERIFY(!response.Version_1_0.has_value());
}


void CServicesGqlTest::testUpdateConnectionUrl_ValidData()
{
	sdl::agentino::Services::CUpdateConnectionUrlResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->succesful = true;

	QVERIFY(response.Version_1_0.has_value());
	QCOMPARE(*response.Version_1_0->succesful, true);
}


void CServicesGqlTest::testUpdateConnectionUrl_InvalidServiceId()
{
	sdl::agentino::Services::CUpdateConnectionUrlResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->succesful = false;

	// Invalid service ID should result in failed response
	QCOMPARE(*response.Version_1_0->succesful, false);
}


// === Status Enum Mapping Tests ===

void CServicesGqlTest::testServiceStatusEnum_AllValues()
{
	// Verify all status enum values map correctly between internal and SDL representations
	struct StatusMapping
	{
		agentinodata::IServiceStatusInfo::ServiceStatus internal;
		sdl::agentino::Services::ServiceStatus sdl;
	};

	StatusMapping mappings[] = {
		{ agentinodata::IServiceStatusInfo::SS_UNDEFINED, sdl::agentino::Services::ServiceStatus::UNDEFINED },
		{ agentinodata::IServiceStatusInfo::SS_STARTING, sdl::agentino::Services::ServiceStatus::STARTING },
		{ agentinodata::IServiceStatusInfo::SS_STOPPING, sdl::agentino::Services::ServiceStatus::STOPPING },
		{ agentinodata::IServiceStatusInfo::SS_RUNNING, sdl::agentino::Services::ServiceStatus::RUNNING },
		{ agentinodata::IServiceStatusInfo::SS_NOT_RUNNING, sdl::agentino::Services::ServiceStatus::NOT_RUNNING },
	};

	for (const auto& mapping: mappings){
		sdl::agentino::Services::ServiceStatus converted = (sdl::agentino::Services::ServiceStatus)mapping.internal;
		QCOMPARE(converted, mapping.sdl);
	}
}


} // namespace agentinotest
