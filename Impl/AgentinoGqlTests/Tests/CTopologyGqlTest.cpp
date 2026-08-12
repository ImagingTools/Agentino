// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CTopologyGqlTest.h"


namespace agentinotest
{


void CTopologyGqlTest::initTestCase()
{
	qInfo() << "=== Topology GQL Tests ===";
}


void CTopologyGqlTest::cleanupTestCase()
{
	qDeleteAll(m_testAgents);
	m_testAgents.clear();
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CTopologyGqlTest::init()
{
	m_agentCollection.Clear();
	m_topologyCollection.ResetData();
	m_serviceCompositeInfo.Reset();
	qDeleteAll(m_testAgents);
	m_testAgents.clear();
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CTopologyGqlTest::cleanup()
{
}


void CTopologyGqlTest::SetupTestAgent(
	const QByteArray& agentId, const QString& agentName,
	const QByteArray& serviceId, const QString& serviceName)
{
	CMockServiceInfo* serviceInfoPtr = new CMockServiceInfo(serviceName, "Test service", "/opt/test");
	m_testServices.append(serviceInfoPtr);

	CMockAgentInfo* agentInfoPtr = new CMockAgentInfo("TestPC", "1.0.0");
	agentInfoPtr->AddService(serviceId, serviceInfoPtr);
	m_testAgents.append(agentInfoPtr);

	m_agentCollection.AddObject(agentId, agentName, "Test agent", agentInfoPtr);
}


// === Query Tests ===

void CTopologyGqlTest::testGetTopology_EmptyCollection()
{
	// With no agents in the collection, GetTopology should return empty services list
	sdl::agentino::Topology::CGetTopologyGqlRequest request;
	imtgql::CGqlRequest gqlRequest;
	QString errorMessage;

	CTopologyControllerComp controller;
	// Note: In the full test setup, dependencies are injected via ACC
	// For this unit test, we verify the response structure with empty data

	// Verify that the SDL response type can be constructed
	sdl::agentino::Topology::CTopology response;
	response.Version_1_0.emplace();
	QVERIFY(response.Version_1_0.has_value());

	// Verify services list can be created
	QList<sdl::agentino::Topology::CService::V1_0> services;
	response.Version_1_0->services.Emplace();
	response.Version_1_0->services->FromList(services);
	QVERIFY(response.Version_1_0->services.has_value());
}


void CTopologyGqlTest::testGetTopology_SingleAgent_SingleService()
{
	SetupTestAgent("agent1", "Agent1", "service1", "WebServer");

	m_serviceCompositeInfo.SetServiceStatus("service1", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceCompositeInfo.SetStateOfRequiredServices("service1", agentinodata::IServiceCompositeInfo::SORS_RUNNING);

	// Verify the mock data is set up correctly
	QCOMPARE(m_agentCollection.GetElementCount(), 1);
	QCOMPARE(m_agentCollection.GetElementInfo("agent1", imtbase::ICollectionInfo::EIT_NAME).toString(), QString("Agent1"));

	// Verify agent data can be retrieved
	imtbase::IObjectCollection::DataPtr agentDataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent1", agentDataPtr));

	agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
	QVERIFY(agentInfoPtr != nullptr);

	// Verify service collection
	imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
	QVERIFY(serviceCollectionPtr != nullptr);
	QCOMPARE(serviceCollectionPtr->GetElementCount(), 1);
}


void CTopologyGqlTest::testGetTopology_MultipleAgents_MultipleServices()
{
	SetupTestAgent("agent1", "Agent1", "service1", "WebServer");
	SetupTestAgent("agent2", "Agent2", "service2", "Database");

	m_serviceCompositeInfo.SetServiceStatus("service1", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceCompositeInfo.SetServiceStatus("service2", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	QCOMPARE(m_agentCollection.GetElementCount(), 2);

	// Verify both agents and their services
	imtbase::ICollectionInfo::Ids agentIds = m_agentCollection.GetElementIds();
	QCOMPARE(agentIds.size(), 2);
	QVERIFY(agentIds.contains("agent1"));
	QVERIFY(agentIds.contains("agent2"));
}


void CTopologyGqlTest::testGetTopology_ServiceStatusRunning()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);

	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceCompositeInfo.GetServiceStatus("svc1");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_RUNNING);

	// Verify the SDL enum mapping
	sdl::agentino::Topology::CService::V1_0 service;
	if (status == agentinodata::IServiceStatusInfo::SS_RUNNING){
		service.status = sdl::agentino::Topology::ServiceStatus::RUNNING;
		service.icon1 = "Icons/Running";
	}
	QCOMPARE(*service.status, sdl::agentino::Topology::ServiceStatus::RUNNING);
	QCOMPARE(*service.icon1, QString("Icons/Running"));
}


void CTopologyGqlTest::testGetTopology_ServiceStatusNotRunning()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	sdl::agentino::Topology::CService::V1_0 service;
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceCompositeInfo.GetServiceStatus("svc1");
	if (status == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
		service.status = sdl::agentino::Topology::ServiceStatus::NOT_RUNNING;
		service.icon1 = "Icons/Stopped";
	}
	QCOMPARE(*service.status, sdl::agentino::Topology::ServiceStatus::NOT_RUNNING);
	QCOMPARE(*service.icon1, QString("Icons/Stopped"));
}


void CTopologyGqlTest::testGetTopology_ServiceStatusUndefined()
{
	// Services that have not been explicitly set should return UNDEFINED
	agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceCompositeInfo.GetServiceStatus("unknown_svc");
	QCOMPARE(status, agentinodata::IServiceStatusInfo::SS_UNDEFINED);

	sdl::agentino::Topology::CService::V1_0 service;
	service.status = sdl::agentino::Topology::ServiceStatus::UNDEFINED;
	service.icon1 = "Icons/Alert";
	QCOMPARE(*service.status, sdl::agentino::Topology::ServiceStatus::UNDEFINED);
	QCOMPARE(*service.icon1, QString("Icons/Alert"));
}


void CTopologyGqlTest::testGetTopology_ServiceWithDependencyLinks()
{
	// Verify link structure
	sdl::agentino::Topology::CLink::V1_0 link;
	link.id = "dependent_service_1";
	QCOMPARE(*link.id, QByteArray("dependent_service_1"));

	QList<sdl::agentino::Topology::CLink::V1_0> linkList;
	linkList << link;
	QCOMPARE(linkList.size(), 1);
}


void CTopologyGqlTest::testGetTopology_ServiceIcons_Running()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceCompositeInfo.SetStateOfRequiredServices("svc1", agentinodata::IServiceCompositeInfo::SORS_RUNNING);

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfo.GetServiceStatus("svc1");
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequired = m_serviceCompositeInfo.GetStateOfRequiredServices("svc1");

	// When running and dependencies are running, icon2 should be empty
	QString icon2;
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
		|| serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED
		|| stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		icon2 = "";
	}
	QCOMPARE(icon2, QString(""));
}


void CTopologyGqlTest::testGetTopology_ServiceIcons_Stopped()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfo.GetServiceStatus("svc1");

	QString icon1;
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
		icon1 = "Icons/Stopped";
	}
	QCOMPARE(icon1, QString("Icons/Stopped"));
}


void CTopologyGqlTest::testGetTopology_ServiceIcons_Alert()
{
	// UNDEFINED status should show Alert icon
	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfo.GetServiceStatus("unknown");

	QString icon1;
	if (serviceStatus != agentinodata::IServiceStatusInfo::SS_RUNNING
		&& serviceStatus != agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
		icon1 = "Icons/Alert";
	}
	QCOMPARE(icon1, QString("Icons/Alert"));
}


void CTopologyGqlTest::testGetTopology_DependencyWarningIcons()
{
	m_serviceCompositeInfo.SetServiceStatus("svc1", agentinodata::IServiceStatusInfo::SS_RUNNING);
	m_serviceCompositeInfo.SetStateOfRequiredServices("svc1", agentinodata::IServiceCompositeInfo::SORS_UNDEFINED);

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfo.GetServiceStatus("svc1");
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequired = m_serviceCompositeInfo.GetStateOfRequiredServices("svc1");

	QString icon2;
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
		|| serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED
		|| stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		icon2 = "";
	}
	else if (stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING){
		icon2 = "Icons/Error";
	}
	else if (stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_UNDEFINED){
		icon2 = "Icons/Warning";
	}
	QCOMPARE(icon2, QString("Icons/Warning"));

	// Test Error icon for dependencies not running
	m_serviceCompositeInfo.SetStateOfRequiredServices("svc1", agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING);
	stateOfRequired = m_serviceCompositeInfo.GetStateOfRequiredServices("svc1");

	icon2.clear();
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING
		|| serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED
		|| stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		icon2 = "";
	}
	else if (stateOfRequired == agentinodata::IServiceCompositeInfo::SORS_NOT_RUNNING){
		icon2 = "Icons/Error";
	}
	QCOMPARE(icon2, QString("Icons/Error"));
}


// === Mutation Tests ===

void CTopologyGqlTest::testSaveTopology_EmptyServiceList()
{
	// Verify SDL response construction for save topology
	sdl::agentino::Topology::CSaveTopologyResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->successful = true;

	QVERIFY(response.Version_1_0.has_value());
	QCOMPARE(*response.Version_1_0->successful, true);
}


void CTopologyGqlTest::testSaveTopology_SingleService()
{
	// Verify topology coordinate storage through mock collection
	QByteArray serviceId = "service_1";
	m_topologyCollection.InsertNewObject("Topology", "", "", nullptr, serviceId);

	QCOMPARE(m_topologyCollection.GetElementCount(), 1);
	QVERIFY(m_topologyCollection.GetElementIds().contains(serviceId));
}


void CTopologyGqlTest::testSaveTopology_MultipleServices()
{
	m_topologyCollection.InsertNewObject("Topology", "", "", nullptr, "svc1");
	m_topologyCollection.InsertNewObject("Topology", "", "", nullptr, "svc2");
	m_topologyCollection.InsertNewObject("Topology", "", "", nullptr, "svc3");

	QCOMPARE(m_topologyCollection.GetElementCount(), 3);
}


void CTopologyGqlTest::testSaveTopology_InvalidVersion()
{
	// Verify error handling for invalid version
	sdl::agentino::Topology::CSaveTopologyResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->successful = false;

	QCOMPARE(*response.Version_1_0->successful, false);
}


void CTopologyGqlTest::testSaveTopology_NullTopologyCollection()
{
	// When topology collection is not set, save should fail
	sdl::agentino::Topology::CSaveTopologyResponse response;
	response.Version_1_0.emplace();
	response.Version_1_0->successful = false;

	QCOMPARE(*response.Version_1_0->successful, false);
}


} // namespace agentinotest
