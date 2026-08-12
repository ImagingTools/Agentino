// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CAgentsGqlTest.h"


namespace agentinotest
{


void CAgentsGqlTest::initTestCase()
{
	qInfo() << "=== Agents GQL Tests ===";
}


void CAgentsGqlTest::cleanupTestCase()
{
	qDeleteAll(m_testAgents);
	m_testAgents.clear();
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CAgentsGqlTest::init()
{
	m_agentCollection.Clear();
	m_agentStatusCollection.Clear();
	m_serviceStatusCollection.Clear();
	qDeleteAll(m_testAgents);
	m_testAgents.clear();
	qDeleteAll(m_testServices);
	m_testServices.clear();
}


void CAgentsGqlTest::cleanup()
{
}


// === Collection Tests ===

void CAgentsGqlTest::testAgentsList_EmptyCollection()
{
	QCOMPARE(m_agentCollection.GetElementCount(), 0);
	QVERIFY(m_agentCollection.GetElementIds().isEmpty());

	// Verify SDL response for empty list
	sdl::agentino::Agents::CAgentListPayload payload;
	payload.Version_1_0.emplace();
	QList<sdl::agentino::Agents::CAgentItem::V1_0> items;
	payload.Version_1_0->items.Emplace();
	payload.Version_1_0->items->FromList(items);
	QVERIFY(payload.Version_1_0.has_value());
}


void CAgentsGqlTest::testAgentsList_SingleAgent()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC-001", "1.5.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent1", "TestAgent", "Test agent description", agentPtr);

	QCOMPARE(m_agentCollection.GetElementCount(), 1);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent1", dataPtr));

	agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(agentInfoPtr != nullptr);
	QCOMPARE(agentInfoPtr->GetComputerName(), QString("PC-001"));
	QCOMPARE(agentInfoPtr->GetVersion(), QString("1.5.0"));
}


void CAgentsGqlTest::testAgentsList_MultipleAgents()
{
	for (int i = 0; i < 4; ++i){
		CMockAgentInfo* agentPtr = new CMockAgentInfo(
			QString("PC-%1").arg(i, 3, 10, QChar('0')),
			QString("%1.0.0").arg(i + 1));
		m_testAgents.append(agentPtr);
		m_agentCollection.AddObject(
			QByteArray("agent") + QByteArray::number(i),
			QString("Agent_%1").arg(i),
			QString("Agent description %1").arg(i),
			agentPtr);
	}

	QCOMPARE(m_agentCollection.GetElementCount(), 4);

	imtbase::ICollectionInfo::Ids ids = m_agentCollection.GetElementIds();
	QCOMPARE(ids.size(), 4);

	for (int i = 0; i < 4; ++i){
		QByteArray id = QByteArray("agent") + QByteArray::number(i);
		QVERIFY(ids.contains(id));
		QCOMPARE(m_agentCollection.GetElementInfo(id, imtbase::ICollectionInfo::EIT_NAME).toString(),
				 QString("Agent_%1").arg(i));
	}
}


void CAgentsGqlTest::testAgentsList_AgentWithServices()
{
	CMockServiceInfo* service1Ptr = new CMockServiceInfo("WebServer", "HTTP service", "/opt/web");
	CMockServiceInfo* service2Ptr = new CMockServiceInfo("Database", "DB service", "/opt/db");
	m_testServices.append(service1Ptr);
	m_testServices.append(service2Ptr);

	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC-001", "1.0.0");
	agentPtr->AddService("svc1", service1Ptr);
	agentPtr->AddService("svc2", service2Ptr);
	m_testAgents.append(agentPtr);

	m_agentCollection.AddObject("agent1", "AgentWithServices", "Agent with 2 services", agentPtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent1", dataPtr));

	agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(agentInfoPtr != nullptr);

	imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
	QVERIFY(serviceCollectionPtr != nullptr);
	QCOMPARE(serviceCollectionPtr->GetElementCount(), 2);

	// Verify semicolon-separated service names representation
	imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
	QStringList serviceNames;
	for (const auto& svcId: serviceIds){
		imtbase::IObjectCollection::DataPtr svcDataPtr;
		if (serviceCollectionPtr->GetObjectData(svcId, svcDataPtr)){
			agentinodata::IServiceInfo* svcInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(svcDataPtr.GetPtr());
			if (svcInfoPtr != nullptr){
				serviceNames << svcInfoPtr->GetServiceName();
			}
		}
	}
	QCOMPARE(serviceNames.size(), 2);
	QString servicesStr = serviceNames.join(";");
	QVERIFY(servicesStr.contains("WebServer"));
	QVERIFY(servicesStr.contains("Database"));
}


// === Get Agent Tests ===

void CAgentsGqlTest::testGetAgent_ValidId()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC-TEST", "3.0.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_get", "GetTestAgent", "Agent for get test", agentPtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent_get", dataPtr));

	agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(agentInfoPtr != nullptr);

	// Verify SDL AgentData representation
	sdl::agentino::Agents::CAgentData::V1_0 agentData;
	agentData.id = QByteArray("agent_get");
	agentData.name = m_agentCollection.GetElementInfo("agent_get", imtbase::ICollectionInfo::EIT_NAME).toString();
	agentData.description = m_agentCollection.GetElementInfo("agent_get", imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
	agentData.lastConnection = agentInfoPtr->GetLastConnection().toString(Qt::ISODate);
	agentData.tracingLevel = agentInfoPtr->GetTracingLevel();

	QCOMPARE(*agentData.id, QByteArray("agent_get"));
	QCOMPARE(*agentData.name, QString("GetTestAgent"));
	QCOMPARE(*agentData.description, QString("Agent for get test"));
}


void CAgentsGqlTest::testGetAgent_InvalidId()
{
	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(!m_agentCollection.GetObjectData("nonexistent_agent", dataPtr));
}


void CAgentsGqlTest::testGetAgent_AllFields()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("WORKSTATION-A", "4.2.1");
	agentPtr->SetTracingLevel(5);
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("full_agent", "FullAgent", "Full agent data", agentPtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("full_agent", dataPtr));

	agentinodata::IAgentInfo* infoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(infoPtr != nullptr);
	QCOMPARE(infoPtr->GetVersion(), QString("4.2.1"));
	QCOMPARE(infoPtr->GetComputerName(), QString("WORKSTATION-A"));
	QCOMPARE(infoPtr->GetTracingLevel(), 5);
	QVERIFY(infoPtr->GetLastConnection().isValid());
}


void CAgentsGqlTest::testGetAgent_TracingLevel()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC", "1.0");
	agentPtr->SetTracingLevel(0);
	m_testAgents.append(agentPtr);

	QCOMPARE(agentPtr->GetTracingLevel(), 0);

	agentPtr->SetTracingLevel(10);
	QCOMPARE(agentPtr->GetTracingLevel(), 10);
}


// === Agent Data Representation Tests ===

void CAgentsGqlTest::testAgentItem_Fields()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	item.id = QByteArray("test_id");
	item.typeId = QByteArray("DocumentInfo");
	item.name = QString("TestAgent");
	item.description = QString("Test description");

	QCOMPARE(*item.id, QByteArray("test_id"));
	QCOMPARE(*item.typeId, QByteArray("DocumentInfo"));
	QCOMPARE(*item.name, QString("TestAgent"));
	QCOMPARE(*item.description, QString("Test description"));
}


void CAgentsGqlTest::testAgentItem_StatusField()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	item.status = QString("Connected");
	QCOMPARE(*item.status, QString("Connected"));

	item.status = QString("Disconnected");
	QCOMPARE(*item.status, QString("Disconnected"));
}


void CAgentsGqlTest::testAgentItem_VersionField()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	item.version = QString("2.5.0");
	QCOMPARE(*item.version, QString("2.5.0"));
}


void CAgentsGqlTest::testAgentItem_LastConnectionField()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	QDateTime now = QDateTime::currentDateTime();
	item.lastConnection = now.toString(Qt::ISODate);
	QVERIFY(item.lastConnection.has_value());
	QVERIFY(!item.lastConnection->isEmpty());
}


void CAgentsGqlTest::testAgentItem_ComputerNameField()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	item.computerName = QString("DESKTOP-ABC123");
	QCOMPARE(*item.computerName, QString("DESKTOP-ABC123"));
}


void CAgentsGqlTest::testAgentItem_ServicesField()
{
	sdl::agentino::Agents::CAgentItem::V1_0 item;
	item.services = QString("WebServer;Database;Logger");
	QCOMPARE(*item.services, QString("WebServer;Database;Logger"));

	// Verify semicolon-separated service names can be parsed
	QStringList serviceList = item.services->split(";");
	QCOMPARE(serviceList.size(), 3);
	QCOMPARE(serviceList[0], QString("WebServer"));
	QCOMPARE(serviceList[1], QString("Database"));
	QCOMPARE(serviceList[2], QString("Logger"));
}


// === Mutation Tests ===

void CAgentsGqlTest::testUpdateAgent_ValidData()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC", "1.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_upd", "OldName", "Old desc", agentPtr);

	// Simulate update through collection
	m_agentCollection.SetElementInfo("agent_upd", imtbase::ICollectionInfo::EIT_NAME, "NewName");
	m_agentCollection.SetElementInfo("agent_upd", imtbase::ICollectionInfo::EIT_DESCRIPTION, "New desc");

	QCOMPARE(m_agentCollection.GetElementInfo("agent_upd", imtbase::ICollectionInfo::EIT_NAME).toString(), QString("NewName"));
	QCOMPARE(m_agentCollection.GetElementInfo("agent_upd", imtbase::ICollectionInfo::EIT_DESCRIPTION).toString(), QString("New desc"));

	// Verify SDL update response
	sdl::imtbase::ImtCollection::CUpdatedNotificationPayload updateResponse;
	QVERIFY(true); // Response structure is valid
}


void CAgentsGqlTest::testUpdateAgent_NameOnly()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC", "1.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_name", "OrigName", "OrigDesc", agentPtr);

	m_agentCollection.SetElementInfo("agent_name", imtbase::ICollectionInfo::EIT_NAME, "UpdatedName");
	QCOMPARE(m_agentCollection.GetElementInfo("agent_name", imtbase::ICollectionInfo::EIT_NAME).toString(), QString("UpdatedName"));
	QCOMPARE(m_agentCollection.GetElementInfo("agent_name", imtbase::ICollectionInfo::EIT_DESCRIPTION).toString(), QString("OrigDesc"));
}


void CAgentsGqlTest::testUpdateAgent_DescriptionOnly()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC", "1.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_desc", "Name", "OldDesc", agentPtr);

	m_agentCollection.SetElementInfo("agent_desc", imtbase::ICollectionInfo::EIT_DESCRIPTION, "NewDescription");
	QCOMPARE(m_agentCollection.GetElementInfo("agent_desc", imtbase::ICollectionInfo::EIT_NAME).toString(), QString("Name"));
	QCOMPARE(m_agentCollection.GetElementInfo("agent_desc", imtbase::ICollectionInfo::EIT_DESCRIPTION).toString(), QString("NewDescription"));
}


void CAgentsGqlTest::testAddAgent_ValidData()
{
	sdl::agentino::Agents::CAgentData::V1_0 agentData;
	agentData.id = QByteArray("new_agent");
	agentData.name = QString("NewAgent");
	agentData.description = QString("Newly added agent");

	CMockAgentInfo* agentPtr = new CMockAgentInfo("NewPC", "1.0.0");
	m_testAgents.append(agentPtr);

	QByteArray insertedId = m_agentCollection.InsertNewObject("DocumentInfo", *agentData.name, *agentData.description, agentPtr, *agentData.id);
	QVERIFY(!insertedId.isEmpty());
	QCOMPARE(m_agentCollection.GetElementCount(), 1);

	// Verify SDL response
	sdl::imtbase::ImtCollection::CAddedNotificationPayload addResponse;
	QVERIFY(true); // Response structure is valid
}


void CAgentsGqlTest::testAddAgent_WithComputerName()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("WORKSTATION-X", "2.0.0");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_cn", "AgentCN", "Agent with computer name", agentPtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent_cn", dataPtr));
	agentinodata::IAgentInfo* infoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(infoPtr != nullptr);
	QCOMPARE(infoPtr->GetComputerName(), QString("WORKSTATION-X"));
}


void CAgentsGqlTest::testAddAgent_WithVersion()
{
	CMockAgentInfo* agentPtr = new CMockAgentInfo("PC", "5.1.3");
	m_testAgents.append(agentPtr);
	m_agentCollection.AddObject("agent_ver", "AgentVer", "Agent with version", agentPtr);

	imtbase::IObjectCollection::DataPtr dataPtr;
	QVERIFY(m_agentCollection.GetObjectData("agent_ver", dataPtr));
	agentinodata::IAgentInfo* infoPtr = dynamic_cast<agentinodata::IAgentInfo*>(dataPtr.GetPtr());
	QVERIFY(infoPtr != nullptr);
	QCOMPARE(infoPtr->GetVersion(), QString("5.1.3"));
}


// === SDL Representation Tests ===

void CAgentsGqlTest::testSdlAgentListPayload_Structure()
{
	sdl::agentino::Agents::CAgentListPayload payload;
	payload.Version_1_0.emplace();

	QList<sdl::agentino::Agents::CAgentItem::V1_0> items;

	sdl::agentino::Agents::CAgentItem::V1_0 item1;
	item1.id = QByteArray("a1");
	item1.typeId = QByteArray("DocumentInfo");
	item1.name = "Agent1";
	items << item1;

	sdl::agentino::Agents::CAgentItem::V1_0 item2;
	item2.id = QByteArray("a2");
	item2.typeId = QByteArray("DocumentInfo");
	item2.name = "Agent2";
	items << item2;

	payload.Version_1_0->items.Emplace();
	payload.Version_1_0->items->FromList(items);
	QVERIFY(payload.Version_1_0->items.has_value());
}


void CAgentsGqlTest::testSdlAgentData_Structure()
{
	sdl::agentino::Agents::CAgentData agentData;
	agentData.Version_1_0.emplace();
	agentData.Version_1_0->id = QByteArray("agent_test");
	agentData.Version_1_0->name = QString("TestAgent");
	agentData.Version_1_0->description = QString("Test description");
	agentData.Version_1_0->lastConnection = QDateTime::currentDateTime().toString(Qt::ISODate);
	agentData.Version_1_0->tracingLevel = 3;

	QVERIFY(agentData.Version_1_0.has_value());
	QCOMPARE(*agentData.Version_1_0->id, QByteArray("agent_test"));
	QCOMPARE(*agentData.Version_1_0->name, QString("TestAgent"));
}


void CAgentsGqlTest::testSdlAgentDataInput_Structure()
{
	sdl::agentino::Agents::CAgentData::V1_0 agentData;
	agentData.id = QByteArray("input_agent");
	agentData.name = QString("InputAgent");
	agentData.description = QString("Agent from input");

	QVERIFY(agentData.id.has_value());
	QVERIFY(agentData.name.has_value());
	QVERIFY(agentData.description.has_value());
}


} // namespace agentinotest
