// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CLocalTopologyInfoComp.h>


// Qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

// ACF includes
#include <istd/CChangeNotifier.h>


namespace agentinodata
{


static const char* const s_topologyFileName = "local_topology.json";
static const char* const s_positionsKey = "positions";
static const char* const s_xKey = "x";
static const char* const s_yKey = "y";


// reimplemented (icomp::CComponentBase)

void CLocalTopologyInfoComp::OnComponentCreated()
{
	LoadFromFile();
}


// reimplemented (agentinodata::ILocalTopologyInfo)

QPoint CLocalTopologyInfoComp::GetServicePosition(const QByteArray& serviceId) const
{
	return m_positions.value(serviceId, QPoint());
}


bool CLocalTopologyInfoComp::SetServicePosition(const QByteArray& serviceId, const QPoint& point)
{
	if (serviceId.isEmpty()){
		return false;
	}

	{
		istd::CChangeNotifier changeNotifier(this);
		m_positions.insert(serviceId, point);
	}

	return SaveToFile();
}


QByteArrayList CLocalTopologyInfoComp::GetServiceIds() const
{
	return m_positions.keys();
}


// reimplemented (istd::IChangeable)

int CLocalTopologyInfoComp::GetSupportedOperations() const
{
	return SO_COPY | SO_RESET;
}


bool CLocalTopologyInfoComp::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const CLocalTopologyInfoComp* sourcePtr = dynamic_cast<const CLocalTopologyInfoComp*>(&object);
	if (sourcePtr == nullptr){
		return false;
	}

	istd::CChangeNotifier changeNotifier(this);
	m_positions = sourcePtr->m_positions;

	return true;
}


istd::IChangeableUniquePtr CLocalTopologyInfoComp::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CLocalTopologyInfoComp);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return nullptr;
}


bool CLocalTopologyInfoComp::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);
	m_positions.clear();

	return true;
}


// private methods

bool CLocalTopologyInfoComp::LoadFromFile()
{
	const QString filePath = GetTopologyFilePath();
	QFile file(filePath);
	if (!file.exists()){
		return true;
	}

	if (!file.open(QIODevice::ReadOnly)){
		SendErrorMessage(0, QString("Unable to load local topology. Error: Cannot open file '%1'").arg(filePath), "CLocalTopologyInfoComp");

		return false;
	}

	const QByteArray data = file.readAll();
	file.close();

	const QJsonDocument document = QJsonDocument::fromJson(data);
	if (!document.isObject()){
		SendErrorMessage(0, QString("Unable to load local topology. Error: File '%1' is not valid JSON").arg(filePath), "CLocalTopologyInfoComp");

		return false;
	}

	const QJsonObject root = document.object();
	const QJsonObject positions = root.value(QLatin1String(s_positionsKey)).toObject();

	m_positions.clear();
	for (auto it = positions.constBegin(); it != positions.constEnd(); ++it){
		const QByteArray serviceId = it.key().toUtf8();
		const QJsonObject posObj = it.value().toObject();
		const int x = posObj.value(QLatin1String(s_xKey)).toInt();
		const int y = posObj.value(QLatin1String(s_yKey)).toInt();
		m_positions.insert(serviceId, QPoint(x, y));
	}

	return true;
}


bool CLocalTopologyInfoComp::SaveToFile() const
{
	const QString filePath = GetTopologyFilePath();

	QJsonObject positions;
	for (auto it = m_positions.constBegin(); it != m_positions.constEnd(); ++it){
		QJsonObject posObj;
		posObj.insert(QLatin1String(s_xKey), it.value().x());
		posObj.insert(QLatin1String(s_yKey), it.value().y());
		positions.insert(QString::fromUtf8(it.key()), posObj);
	}

	QJsonObject root;
	root.insert(QLatin1String(s_positionsKey), positions);

	const QJsonDocument document(root);

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
		SendErrorMessage(0, QString("Unable to save local topology. Error: Cannot open file '%1' for writing").arg(filePath), "CLocalTopologyInfoComp");

		return false;
	}

	file.write(document.toJson(QJsonDocument::Indented));
	file.close();

	return true;
}


QString CLocalTopologyInfoComp::GetTopologyFilePath() const
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String(s_topologyFileName);
}


} // namespace agentinodata
