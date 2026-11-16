// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArrayList>

// ACF includes
#include <i2d/CPosition2d.h>


namespace agentinodata
{


/**
	User-defined group of the service topology (composite node).
	The position of the group on the topology scheme is inherited from i2d::CPosition2d,
	so the layout code handling plain positions works for groups as well.
	The group holds the IDs of its direct members (services, agents or other groups).
	The display name of the group is the element name in the topology collection.
*/
class CTopologyGroup: public i2d::CPosition2d
{
public:
	typedef i2d::CPosition2d BaseClass;

	static QByteArray GetTypeName();

	CTopologyGroup();

	/**
		Get IDs of the direct members of the group.
	*/
	const QByteArrayList& GetMemberIds() const;

	/**
		Set IDs of the direct members of the group.
	*/
	void SetMemberIds(const QByteArrayList& memberIds);

	// reimplemented (iser::IObject)
	virtual QByteArray GetFactoryId() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual bool IsEqual(const istd::IChangeable& object) const override;
	virtual istd::TUniqueInterfacePtr<istd::IChangeable> CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;

protected:
	QByteArrayList m_memberIds;
};


// public inline methods

inline const QByteArrayList& CTopologyGroup::GetMemberIds() const
{
	return m_memberIds;
}


} // namespace agentinodata


