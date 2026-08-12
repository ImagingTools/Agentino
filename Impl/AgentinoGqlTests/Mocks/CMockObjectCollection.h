// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QMap>
#include <QtCore/QVariant>

// ImtCore includes
#include <imtbase/IObjectCollection.h>


namespace agentinotest
{

/**
	Mock implementation of IObjectCollection for testing GQL controllers.
	Provides a simple in-memory key-value store for test objects.
*/
class CMockObjectCollection: virtual public imtbase::IObjectCollection
{
public:
	CMockObjectCollection() = default;

	// Helper methods for populating test data
	void AddObject(const Id& id, const QString& name, const QString& description, istd::IChangeable* objectPtr)
	{
		ObjectEntry entry;
		entry.name = name;
		entry.description = description;
		entry.objectPtr = objectPtr;
		m_objects[id] = entry;
	}

	void RemoveObject(const Id& id)
	{
		m_objects.remove(id);
	}

	void Clear()
	{
		m_objects.clear();
	}

	// reimplemented (imtbase::IObjectCollection)
	virtual QByteArray InsertNewObject(
				const QByteArray& typeId,
				const QString& name,
				const QString& description,
				const istd::IChangeable* objectPtr = nullptr,
				const Id& objectId = Id()) override
	{
		QByteArray id = objectId.isEmpty() ? QByteArray::number(m_nextId++) : objectId;
		ObjectEntry entry;
		entry.typeId = typeId;
		entry.name = name;
		entry.description = description;
		if (objectPtr != nullptr){
			// In mock, we store a reference - caller must manage lifetime
			entry.objectPtr = const_cast<istd::IChangeable*>(objectPtr);
		}
		m_objects[id] = entry;
		return id;
	}

	virtual bool RemoveObject(const Id& id, bool /*beQuiet*/ = false) override
	{
		return m_objects.remove(id) > 0;
	}

	virtual bool RemoveObjects(const Ids& ids, bool /*beQuiet*/ = false) override
	{
		bool allRemoved = true;
		for (const Id& id: ids){
			if (m_objects.remove(id) == 0){
				allRemoved = false;
			}
		}
		return allRemoved;
	}

	virtual bool GetObjectData(const Id& id, DataPtr& dataPtr) const override
	{
		if (m_objects.contains(id) && m_objects[id].objectPtr != nullptr){
			dataPtr = DataPtr(m_objects[id].objectPtr);
			return true;
		}
		return false;
	}

	virtual bool SetObjectData(const Id& /*id*/, const istd::IChangeable& /*data*/, bool /*beQuiet*/ = false) override
	{
		return true;
	}

	// reimplemented (imtbase::ICollectionInfo)
	virtual Ids GetElementIds() const override
	{
		return Ids(m_objects.keys());
	}

	virtual int GetElementCount() const override
	{
		return m_objects.count();
	}

	virtual QVariant GetElementInfo(const Id& id, int infoId) const override
	{
		if (!m_objects.contains(id)){
			return QVariant();
		}

		const ObjectEntry& entry = m_objects[id];
		switch(infoId){
			case EIT_NAME:
				return entry.name;
			case EIT_DESCRIPTION:
				return entry.description;
			case EIT_TYPE_ID:
				return entry.typeId;
			default:
				return QVariant();
		}
	}

	virtual bool SetElementInfo(const Id& id, int infoId, const QVariant& value) override
	{
		if (!m_objects.contains(id)){
			return false;
		}

		ObjectEntry& entry = m_objects[id];
		switch(infoId){
			case EIT_NAME:
				entry.name = value.toString();
				return true;
			case EIT_DESCRIPTION:
				entry.description = value.toString();
				return true;
			default:
				return false;
		}
	}

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override { return 0; }
	virtual bool CopyFrom(const IChangeable& /*object*/, CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override { return false; }
	virtual bool ResetData(CompatibilityMode /*mode*/ = CM_WITHOUT_REFS) override
	{
		m_objects.clear();
		return true;
	}

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& /*archive*/) override { return true; }

private:
	struct ObjectEntry
	{
		QByteArray typeId;
		QString name;
		QString description;
		istd::IChangeable* objectPtr = nullptr;
	};

	QMap<Id, ObjectEntry> m_objects;
	int m_nextId = 1;
};

} // namespace agentinotest
