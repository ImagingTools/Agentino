// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "CServiceLogGqlTest.h"


namespace agentinotest
{


void CServiceLogGqlTest::initTestCase()
{
	qInfo() << "=== ServiceLog GQL Tests ===";
}


void CServiceLogGqlTest::cleanupTestCase()
{
}


void CServiceLogGqlTest::init()
{
	m_serviceCollection.Clear();
}


void CServiceLogGqlTest::cleanup()
{
}


// === Message Item Tests ===

void CServiceLogGqlTest::testMessageItem_Fields()
{
	sdl::imtbase::ImtCollection::CMessageItem::V1_0 message;
	message.id = QByteArray("msg_1");
	message.name = QString("Test message");

	QVERIFY(message.id.has_value());
	QCOMPARE(*message.id, QByteArray("msg_1"));
	QCOMPARE(*message.name, QString("Test message"));
}


void CServiceLogGqlTest::testMessageItem_AllCategories()
{
	// Test different message categories
	QStringList categories = {"Info", "Warning", "Error", "Debug", "Trace"};

	for (const QString& category: categories){
		sdl::imtbase::ImtCollection::CMessageItem::V1_0 message;
		message.id = QByteArray("msg_") + category.toLatin1();
		message.name = QString("Message: %1").arg(category);
		message.description = category;

		QVERIFY(message.id.has_value());
		QCOMPARE(*message.description, category);
	}
}


void CServiceLogGqlTest::testMessageItem_TimestampFormat()
{
	QDateTime now = QDateTime::currentDateTime();
	QString isoTimestamp = now.toString(Qt::ISODate);

	// Verify ISO date format is valid
	QDateTime parsed = QDateTime::fromString(isoTimestamp, Qt::ISODate);
	QVERIFY(parsed.isValid());

	sdl::imtbase::ImtCollection::CMessageItem::V1_0 message;
	message.id = QByteArray("msg_ts");
	message.name = isoTimestamp;
	QVERIFY(!message.name->isEmpty());
}


// === Collection Tests ===

void CServiceLogGqlTest::testServiceLogCollection_Empty()
{
	QCOMPARE(m_serviceCollection.GetElementCount(), 0);
}


void CServiceLogGqlTest::testServiceLogCollection_WithMessages()
{
	// Add mock messages to collection
	for (int i = 0; i < 5; ++i){
		m_serviceCollection.InsertNewObject(
			"Message",
			QString("Log message %1").arg(i),
			QString("Details for message %1").arg(i),
			nullptr,
			QByteArray("msg_") + QByteArray::number(i));
	}

	QCOMPARE(m_serviceCollection.GetElementCount(), 5);

	// Verify each message
	for (int i = 0; i < 5; ++i){
		QByteArray id = QByteArray("msg_") + QByteArray::number(i);
		QVERIFY(m_serviceCollection.GetElementIds().contains(id));
		QCOMPARE(m_serviceCollection.GetElementInfo(id, imtbase::ICollectionInfo::EIT_NAME).toString(),
				 QString("Log message %1").arg(i));
	}
}


void CServiceLogGqlTest::testServiceLogCollection_Pagination()
{
	// Add 20 messages
	for (int i = 0; i < 20; ++i){
		m_serviceCollection.InsertNewObject(
			"Message",
			QString("Message_%1").arg(i, 3, 10, QChar('0')),
			QString("Description %1").arg(i),
			nullptr,
			QByteArray("msg_") + QByteArray::number(i));
	}

	QCOMPARE(m_serviceCollection.GetElementCount(), 20);

	// Simulate pagination
	imtbase::ICollectionInfo::Ids allIds = m_serviceCollection.GetElementIds();
	int pageSize = 10;
	int totalPages = (allIds.size() + pageSize - 1) / pageSize;
	QCOMPARE(totalPages, 2);

	QList<QByteArray> page1 = allIds.mid(0, pageSize);
	QCOMPARE(page1.size(), 10);

	QList<QByteArray> page2 = allIds.mid(pageSize, pageSize);
	QCOMPARE(page2.size(), 10);
}


// === SDL Structure Tests ===

void CServiceLogGqlTest::testMessageListPayload_Structure()
{
	sdl::imtbase::ImtCollection::CMessageListPayload payload;
	payload.Version_1_0.emplace();

	QVERIFY(payload.Version_1_0.has_value());
}


void CServiceLogGqlTest::testMessageListPayload_EmptyList()
{
	sdl::imtbase::ImtCollection::CMessageListPayload payload;
	payload.Version_1_0.emplace();

	QList<sdl::imtbase::ImtCollection::CMessageItem::V1_0> items;
	payload.Version_1_0->items.Emplace();
	payload.Version_1_0->items->FromList(items);

	QVERIFY(payload.Version_1_0->items.has_value());
}


void CServiceLogGqlTest::testMessageListPayload_MultipleMessages()
{
	QList<sdl::imtbase::ImtCollection::CMessageItem::V1_0> items;

	for (int i = 0; i < 3; ++i){
		sdl::imtbase::ImtCollection::CMessageItem::V1_0 item;
		item.id = QByteArray("msg_") + QByteArray::number(i);
		item.name = QString("Message %1").arg(i);
		items << item;
	}

	QCOMPARE(items.size(), 3);
	QCOMPARE(*items[0].id, QByteArray("msg_0"));
	QCOMPARE(*items[1].id, QByteArray("msg_1"));
	QCOMPARE(*items[2].id, QByteArray("msg_2"));
}


} // namespace agentinotest
