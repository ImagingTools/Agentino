// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArrayList>
#include <QtCore/QMap>

// ACF includes
#include <iser/IObject.h>
#include <i2d/CVector2d.h>


namespace agentinodata
{


/**
	User-defined view of the service topology: a way to group the topology nodes independently
	of agents. The view holds the IDs of its groups (agentinodata::CTopologyGroup elements of the
	same topology collection) and the positions of the services on its scheme.
	The display name of the view is the element name in the topology collection.
*/
class CTopologyView: virtual public iser::IObject
{
public:
	static QByteArray GetTypeName();

	CTopologyView();

	/**
		Get IDs of the groups belonging to the view.
	*/
	const QByteArrayList& GetGroupIds() const;

	/**
		Set IDs of the groups belonging to the view.
	*/
	void SetGroupIds(const QByteArrayList& groupIds);

	/**
		Get the position of the node on the scheme of the view.
		\return false if no position is stored for the node.
	*/
	bool GetNodePosition(const QByteArray& nodeId, i2d::CVector2d& position) const;

	/**
		Set the position of the node on the scheme of the view.
	*/
	void SetNodePosition(const QByteArray& nodeId, const i2d::CVector2d& position);

	/**
		Get IDs of the services removed from the view. Such services are not shown in the view,
		but they still exist and are shown in other views.
	*/
	const QByteArrayList& GetRemovedNodeIds() const;

	/**
		Set IDs of the services removed from the view.
	*/
	void SetRemovedNodeIds(const QByteArrayList& nodeIds);

	// reimplemented (iser::IObject)
	virtual QByteArray GetFactoryId() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual bool IsEqual(const istd::IChangeable& object) const override;
	virtual istd::TUniqueInterfacePtr<istd::IChangeable> CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	QByteArrayList m_groupIds;
	QMap<QByteArray, i2d::CVector2d> m_nodePositions;
	QByteArrayList m_removedNodeIds;
};


// public inline methods

inline const QByteArrayList& CTopologyView::GetGroupIds() const
{
	return m_groupIds;
}


inline const QByteArrayList& CTopologyView::GetRemovedNodeIds() const
{
	return m_removedNodeIds;
}


} // namespace agentinodata


