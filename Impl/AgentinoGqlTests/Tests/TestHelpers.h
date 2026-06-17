// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>

/**
	Test utility helpers for validating GQL responses and constructing GQL requests.
*/
namespace agentinotest
{


/**
	Helper to build a GraphQL query JSON body.
*/
inline QJsonObject MakeGqlQuery(const QString& query, const QJsonObject& variables = QJsonObject())
{
	QJsonObject body;
	body["query"] = query;
	if (!variables.isEmpty()){
		body["variables"] = variables;
	}
	return body;
}


/**
	Helper to verify that a JSON response has data and no errors.
*/
inline bool VerifyNoErrors(const QJsonObject& response, QString& errorMessage)
{
	if (response.contains("errors")){
		QJsonArray errors = response["errors"].toArray();
		if (!errors.isEmpty()){
			errorMessage = QJsonDocument(errors).toJson(QJsonDocument::Compact);
			return false;
		}
	}
	return true;
}


/**
	Helper to extract data from a GQL response.
*/
inline QJsonObject GetResponseData(const QJsonObject& response)
{
	return response["data"].toObject();
}


/**
	Helper to verify a JSON object has expected string field.
*/
inline bool VerifyStringField(const QJsonObject& obj, const QString& field, const QString& expected, QString& errorMessage)
{
	if (!obj.contains(field)){
		errorMessage = QString("Missing field: %1").arg(field);
		return false;
	}
	QString actual = obj[field].toString();
	if (actual != expected){
		errorMessage = QString("Field '%1': expected '%2', got '%3'").arg(field, expected, actual);
		return false;
	}
	return true;
}


/**
	Helper to verify a JSON object has expected boolean field.
*/
inline bool VerifyBoolField(const QJsonObject& obj, const QString& field, bool expected, QString& errorMessage)
{
	if (!obj.contains(field)){
		errorMessage = QString("Missing field: %1").arg(field);
		return false;
	}
	bool actual = obj[field].toBool();
	if (actual != expected){
		errorMessage = QString("Field '%1': expected '%2', got '%3'").arg(field).arg(expected).arg(actual);
		return false;
	}
	return true;
}


/**
	Helper to verify JSON array has expected size.
*/
inline bool VerifyArraySize(const QJsonArray& arr, int expectedSize, const QString& context, QString& errorMessage)
{
	if (arr.size() != expectedSize){
		errorMessage = QString("%1: expected %2 items, got %3").arg(context).arg(expectedSize).arg(arr.size());
		return false;
	}
	return true;
}


/**
	Helper class for test result tracking and reporting.
*/
class CTestReporter
{
public:
	CTestReporter(const QString& suiteName)
		: m_suiteName(suiteName)
		, m_passed(0)
		, m_failed(0)
	{
	}

	void ReportPass(const QString& testName)
	{
		m_passed++;
		qInfo().noquote() << QString("  [PASS] %1::%2").arg(m_suiteName, testName);
	}

	void ReportFail(const QString& testName, const QString& message)
	{
		m_failed++;
		qWarning().noquote() << QString("  [FAIL] %1::%2 - %3").arg(m_suiteName, testName, message);
	}

	void ReportSkip(const QString& testName, const QString& reason)
	{
		qInfo().noquote() << QString("  [SKIP] %1::%2 - %3").arg(m_suiteName, testName, reason);
	}

	int GetPassedCount() const { return m_passed; }
	int GetFailedCount() const { return m_failed; }
	int GetTotalCount() const { return m_passed + m_failed; }

	void PrintSummary() const
	{
		qInfo().noquote() << QString("\n  %1 Summary: %2 passed, %3 failed, %4 total")
			.arg(m_suiteName)
			.arg(m_passed)
			.arg(m_failed)
			.arg(m_passed + m_failed);
	}

private:
	QString m_suiteName;
	int m_passed;
	int m_failed;
};


} // namespace agentinotest
