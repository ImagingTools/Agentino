// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CServerServiceGqlTest.h"

// Mock includes
#include "../Mocks/CMockServiceInfo.h"
#include "../Mocks/CMockAgentInfo.h"


namespace agentinotest
{


void CServerServiceGqlTest::initTestCase()
{
	qInfo() << "=== Server Service GQL Tests ===";
}


void CServerServiceGqlTest::cleanupTestCase()
{
}


void CServerServiceGqlTest::init()
{
	m_serviceController.Reset();
	m_serviceManager.Reset();
	m_serviceCollection.Clear();
	m_serviceStatusCollection.Clear();
	m_agentCollection.Clear();
	m_serviceCompositeInfo.Reset();
}


void CServerServiceGqlTest::cleanup()
{
}


// === ServiceControllerProxy Tests ===

void CServerServiceGqlTest::testProxy_StartService_SetsStatusViaManager()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	// Start service through mock
	m_serviceController.StartService("svc1");
	QCOMPARE(m_serviceController.GetServiceStatus("svc1"), agentinodata::IServiceStatusInfo::SS_RUNNING);

	// Verify the call was recorded
	QCOMPARE(m_serviceController.GetStartCalls().size(), 1);
	QCOMPARE(m_serviceController.GetStartCalls().first(), QByteArray("svc1"));

	// Map to SDL response
	sdl::agentino::Services::CServiceStatusResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus)m_serviceController.GetServiceStatus("svc1");
	QCOMPARE(*response.Version_1_0->status, sdl::agentino::Services::ServiceStatus::RUNNING);
}


void CServerServiceGqlTest::testProxy_StopService_SetsStatusViaManager()
{
	m_serviceController.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);

	m_serviceController.StopService("svc1");
	QCOMPARE(m_serviceController.GetServiceStatus("svc1"), agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	QCOMPARE(m_serviceController.GetStopCalls().size(), 1);
	QCOMPARE(m_serviceController.GetStopCalls().first(), QByteArray("svc1"));
}


void CServerServiceGqlTest::testProxy_GetServiceStatus_ReturnsCorrectStatus()
{
	m_serviceController.SetServiceStatus("svc_running", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceController.SetServiceStatus("svc_stopped", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
	m_serviceController.SetServiceStatus("svc_starting", agentinodata::IServiceStatusInfo::SS_STARTING);

	QCOMPARE(m_serviceController.GetServiceStatus("svc_running"), agentinodata::IServiceStatusInfo::SS_RUNNING);
	QCOMPARE(m_serviceController.GetServiceStatus("svc_stopped"), agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
	QCOMPARE(m_serviceController.GetServiceStatus("svc_starting"), agentinodata::IServiceStatusInfo::SS_STARTING);
	QCOMPARE(m_serviceController.GetServiceStatus("unknown"), agentinodata::IServiceStatusInfo::SS_UNDEFINED);
}


void CServerServiceGqlTest::testProxy_GetServiceStatus_AllStatusValues()
{
	QMap<QByteArray, agentinodata::IServiceStatusInfo::ServiceStatus> statuses;
	statuses["s1"] = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	statuses["s2"] = agentinodata::IServiceStatusInfo::SS_STARTING;
	statuses["s3"] = agentinodata::IServiceStatusInfo::SS_STOPPING;
	statuses["s4"] = agentinodata::IServiceStatusInfo::SS_RUNNING;
	statuses["s5"] = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
	statuses["s6"] = agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE;

	for (auto it = statuses.constBegin(); it != statuses.constEnd(); ++it){
		m_serviceController.SetServiceStatus(it.key(), it.value());
		QCOMPARE(m_serviceController.GetServiceStatus(it.key()), it.value());
	}
}


// === ServerServiceCollection Tests ===

void CServerServiceGqlTest::testServerCollection_EmptyList()
{
	QCOMPARE(m_serviceCollection.GetElementCount(), 0);
	QVERIFY(m_serviceCollection.GetElementIds().isEmpty());
}


void CServerServiceGqlTest::testServerCollection_ServiceItemFields()
{
	sdl::agentino::Services::CServiceItem::V1_0 item;
	item.id = QByteArray("svc_item1");
	item.typeId = QByteArray("Service");
	item.name = QString("WebAPI");
	item.description = QString("REST API service");
	item.path = QString("/opt/webapi");
	item.status = QString("RUNNING");
	item.version = QString("3.2.0");

	QCOMPARE(*item.id, QByteArray("svc_item1"));
	QCOMPARE(*item.typeId, QByteArray("Service"));
	QCOMPARE(*item.name, QString("WebAPI"));
	QCOMPARE(*item.description, QString("REST API service"));
	QCOMPARE(*item.path, QString("/opt/webapi"));
	QCOMPARE(*item.status, QString("RUNNING"));
	QCOMPARE(*item.version, QString("3.2.0"));
}


void CServerServiceGqlTest::testServerCollection_MultipleServices_StatusMapping()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceCompositeInfo.SetServiceStatus("svc2", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
	m_serviceCompositeInfo.SetServiceStatus("svc3", agentinodata::IServiceStatusInfo::SS_UNDEFINED);

	// Verify status retrieval
	QCOMPARE(m_serviceCompositeInfo.GetServiceStatus("svc1"), agentinodata::IServiceStatusInfo::SS_RUNNING);
	QCOMPARE(m_serviceCompositeInfo.GetServiceStatus("svc2"), agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
	QCOMPARE(m_serviceCompositeInfo.GetServiceStatus("svc3"), agentinodata::IServiceStatusInfo::SS_UNDEFINED);
}


void CServerServiceGqlTest::testServerCollection_DependencyStatusInfo()
{
	m_serviceCompositeInfo.SetServiceName("svc1", "WebServer");
	m_serviceCompositeInfo.SetServiceAgentName("svc1", "Agent1");
	m_serviceCompositeInfo.SetStateOfRequiredServices("svc1", agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING);

	QByteArray depServiceId = "svc_dep1";
	agentinodata::IServiceCompositeInfo::Ids deps;
	deps << depServiceId;
	m_serviceCompositeInfo.SetDependencyServices("svc1", deps);

	QCOMPARE(m_serviceCompositeInfo.GetServiceName("svc1"), QString("WebServer"));
	QCOMPARE(m_serviceCompositeInfo.GetServiceAgentName("svc1"), QString("Agent1"));
	QCOMPARE(m_serviceCompositeInfo.GetStateOfRequiredServices("svc1"), agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING);
	QCOMPARE(m_serviceCompositeInfo.GetDependencyServices("svc1").size(), 1);
	QVERIFY(m_serviceCompositeInfo.GetDependencyServices("svc1").contains(depServiceId));
}


void CServerServiceGqlTest::testServerCollection_CompositeServiceInfo()
{
	QByteArray connectionId = "conn1";
	QByteArray serviceId = "svc_target";

	m_serviceCompositeInfo.SetServiceIdForConnection(connectionId, serviceId);
	QCOMPARE(m_serviceCompositeInfo.GetServiceId(connectionId), serviceId);

	// Non-existent connection should return empty
	QVERIFY(m_serviceCompositeInfo.GetServiceId("nonexistent").isEmpty());
}


// === ServiceManager Integration Tests ===

void CServerServiceGqlTest::testServiceManager_AddService()
{
	CMockServiceInfo serviceInfo("NewService", "New service description", "/opt/new");
	m_serviceManager.AddService("agent1", serviceInfo, "svc_new", "NewService", "New service description");

	QCOMPARE(m_serviceManager.GetAddCallCount(), 1);
	QCOMPARE(m_serviceManager.GetServiceCount(), 1);
}


void CServerServiceGqlTest::testServiceManager_RemoveService()
{
	CMockServiceInfo serviceInfo("ToRemove", "Remove me", "/opt/rm");
	m_serviceManager.AddService("agent1", serviceInfo, "svc_rm");

	imtbase::ICollectionInfo::Ids idsToRemove;
	idsToRemove << "svc_rm";
	m_serviceManager.RemoveServices("agent1", idsToRemove);

	QCOMPARE(m_serviceManager.GetRemoveCallCount(), 1);
	QCOMPARE(m_serviceManager.GetServiceCount(), 0);
}


void CServerServiceGqlTest::testServiceManager_SetService()
{
	CMockServiceInfo serviceInfo("Original", "Original desc", "/opt/orig");
	m_serviceManager.AddService("agent1", serviceInfo, "svc_set");

	CMockServiceInfo updatedInfo("Updated", "Updated desc", "/opt/updated");
	bool result = m_serviceManager.SetService("agent1", "svc_set", updatedInfo, "Updated", "Updated desc");

	QVERIFY(result);
	QCOMPARE(m_serviceManager.GetSetCallCount(), 1);
}


void CServerServiceGqlTest::testServiceManager_ServiceExists()
{
	CMockServiceInfo serviceInfo("Existing", "Exists", "/opt/exists");
	m_serviceManager.AddService("agent1", serviceInfo, "svc_exists");

	QVERIFY(m_serviceManager.ServiceExists("agent1", "svc_exists"));
	QVERIFY(!m_serviceManager.ServiceExists("agent1", "nonexistent"));
}


// === SDL Response Structure Tests ===

void CServerServiceGqlTest::testSdlServiceItem_AllFields()
{
	sdl::agentino::Services::CServiceItem::V1_0 item;
	item.id = QByteArray("full_item");
	item.typeId = QByteArray("FullService");
	item.name = QString("FullService");
	item.description = QString("Full service with all fields");
	item.path = QString("/opt/full");
	item.status = QString("RUNNING");
	item.version = QString("4.0.0");

	QVERIFY(item.id.has_value());
	QVERIFY(item.typeId.has_value());
	QVERIFY(item.name.has_value());
	QVERIFY(item.description.has_value());
	QVERIFY(item.path.has_value());
	QVERIFY(item.status.has_value());
	QVERIFY(item.version.has_value());
}


void CServerServiceGqlTest::testSdlServiceData_WithConnections()
{
	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	serviceData.id = QByteArray("svc_conn");
	serviceData.name = QString("ServiceWithConnections");
	serviceData.path = QString("/opt/connected");
	serviceData.serviceTypeId = QString("Gateway");
	serviceData.isAutoStart = true;
	serviceData.tracingLevel = 2;
	serviceData.startScript = QString("/opt/connected/start.sh");
	serviceData.stopScript = QString("/opt/connected/stop.sh");
	serviceData.arguments = QString("--port=9090 --host=0.0.0.0");
	serviceData.enableVerbose = true;
	serviceData.status = QString("RUNNING");

	QCOMPARE(*serviceData.id, QByteArray("svc_conn"));
	QCOMPARE(*serviceData.serviceTypeId, QString("Gateway"));
	QCOMPARE(*serviceData.isAutoStart, true);
	QCOMPARE(*serviceData.tracingLevel, 2);
	QCOMPARE(*serviceData.enableVerbose, true);
}


void CServerServiceGqlTest::testSdlServiceListPayload_Structure()
{
	sdl::agentino::Services::CServiceListPayload payload;
	payload.Version_1_0.emplace();

	QList<sdl::agentino::Services::CServiceItem::V1_0> items;
	for (int i = 0; i < 3; ++i){
		sdl::agentino::Services::CServiceItem::V1_0 item;
		item.id = QByteArray("item_") + QByteArray::number(i);
		item.typeId = QByteArray("Service");
		item.name = QString("Service_%1").arg(i);
		items << item;
	}

	payload.Version_1_0->items.Emplace();
	payload.Version_1_0->items->FromList(items);
	QVERIFY(payload.Version_1_0->items.has_value());
}


void CServerServiceGqlTest::testSdlUpdateConnectionUrlResponse_Success()
{
	sdl::agentino::Services::CUpdateConnectionUrlResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->succesful = true;

	QVERIFY(response.Version_1_0.has_value());
	QCOMPARE(*response.Version_1_0->succesful, true);
}


void CServerServiceGqlTest::testSdlUpdateConnectionUrlResponse_Failure()
{
	sdl::agentino::Services::CUpdateConnectionUrlResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->succesful = false;

	QCOMPARE(*response.Version_1_0->succesful, false);
}


void CServerServiceGqlTest::testSdlPluginInfo_Structure()
{
	sdl::agentino::Services::CPluginInfo pluginInfo;
	pluginInfo.Version_1_0.emplace();
	pluginInfo.Version_1_0->servicePath = QString("/opt/plugin/path");

	QVERIFY(pluginInfo.Version_1_0.has_value());
	QCOMPARE(*pluginInfo.Version_1_0->servicePath, QString("/opt/plugin/path"));
}


} // namespace agentinotest
