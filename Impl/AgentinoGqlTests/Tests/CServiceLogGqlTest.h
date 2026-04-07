// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtTest/QtTest>
#include <QtCore/QJsonObject>

// Test includes
#include "TestHelpers.h"

// Mock includes
#include "../Mocks/CMockObjectCollection.h"

// Agentino GQL includes
#include <agentgql/CServiceLogControllerComp.h>
#include <agentgql/CMessageCollectionControllerComp.h>

// SDL includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/ServiceLog.h>
#include <imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


namespace agentinotest
{


/**
	Test suite for ServiceLog GQL queries:
	- GetServiceLog
	- MessageItem representation
*/
class CServiceLogGqlTest: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Message item tests
	void testMessageItem_Fields();
	void testMessageItem_AllCategories();
	void testMessageItem_TimestampFormat();

	// Collection tests
	void testServiceLogCollection_Empty();
	void testServiceLogCollection_WithMessages();
	void testServiceLogCollection_Pagination();

	// SDL structure tests
	void testMessageListPayload_Structure();
	void testMessageListPayload_EmptyList();
	void testMessageListPayload_MultipleMessages();

private:
	CMockObjectCollection m_serviceCollection;
};


} // namespace agentinotest
