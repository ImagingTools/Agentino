// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CTopologyView.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/CClassInfo.h>
#include <iser/IArchive.h>
#include <iser/CPrimitiveTypesSerializer.h>


namespace agentinodata
{


// public static methods

QByteArray CTopologyView::GetTypeName()
{
	return istd::CClassInfo::GetName<CTopologyView>();
}


// public methods

CTopologyView::CTopologyView()
{
}


void CTopologyView::SetGroupIds(const QByteArrayList& groupIds)
{
	if (m_groupIds != groupIds){
		istd::CChangeNotifier changeNotifier(this);
		Q_UNUSED(changeNotifier);

		m_groupIds = groupIds;
	}
}


bool CTopologyView::GetNodePosition(const QByteArray& nodeId, i2d::CVector2d& position) const
{
	QMap<QByteArray, i2d::CVector2d>::const_iterator iter = m_nodePositions.constFind(nodeId);
	if (iter == m_nodePositions.constEnd()){
		return false;
	}

	position = iter.value();

	return true;
}


void CTopologyView::SetNodePosition(const QByteArray& nodeId, const i2d::CVector2d& position)
{
	QMap<QByteArray, i2d::CVector2d>::const_iterator iter = m_nodePositions.constFind(nodeId);
	if ((iter == m_nodePositions.constEnd()) || (iter.value() != position)){
		istd::CChangeNotifier changeNotifier(this);
		Q_UNUSED(changeNotifier);

		m_nodePositions.insert(nodeId, position);
	}
}


void CTopologyView::SetRemovedNodeIds(const QByteArrayList& nodeIds)
{
	if (m_removedNodeIds != nodeIds){
		istd::CChangeNotifier changeNotifier(this);
		Q_UNUSED(changeNotifier);

		m_removedNodeIds = nodeIds;
	}
}


// reimplemented (iser::IObject)

QByteArray CTopologyView::GetFactoryId() const
{
	return GetTypeName();
}


// reimplemented (iser::ISerializable)

bool CTopologyView::Serialize(iser::IArchive& archive)
{
	istd::CChangeNotifier changeNotifier(archive.IsStoring()? nullptr: this, &GetAllChanges());
	Q_UNUSED(changeNotifier);

	// Positions are stored as parallel lists of node IDs and coordinates.
	QByteArrayList nodeIds;
	QList<double> positionsX;
	QList<double> positionsY;
	if (archive.IsStoring()){
		for (QMap<QByteArray, i2d::CVector2d>::const_iterator iter = m_nodePositions.constBegin(); iter != m_nodePositions.constEnd(); ++iter){
			nodeIds.append(iter.key());
			positionsX.append(iter.value().GetX());
			positionsY.append(iter.value().GetY());
		}
	}

	bool retVal = iser::CPrimitiveTypesSerializer::SerializeContainer(archive, m_groupIds, "Groups", "GroupId");
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, nodeIds, "PositionNodes", "NodeId");
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, positionsX, "PositionsX", "X");
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, positionsY, "PositionsY", "Y");
	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, m_removedNodeIds, "RemovedNodes", "NodeId");

	if (retVal && !archive.IsStoring()){
		if ((nodeIds.count() != positionsX.count()) || (nodeIds.count() != positionsY.count())){
			return false;
		}

		m_nodePositions.clear();
		for (int i = 0; i < nodeIds.count(); ++i){
			m_nodePositions.insert(nodeIds[i], i2d::CVector2d(positionsX[i], positionsY[i]));
		}
	}

	return retVal;
}


// reimplemented (istd::IChangeable)

int CTopologyView::GetSupportedOperations() const
{
	return SO_COPY | SO_CLONE | SO_COMPARE | SO_RESET;
}


bool CTopologyView::CopyFrom(const IChangeable& object, CompatibilityMode /*mode*/)
{
	const CTopologyView* viewPtr = dynamic_cast<const CTopologyView*>(&object);
	if (viewPtr != nullptr){
		istd::CChangeNotifier changeNotifier(this);
		Q_UNUSED(changeNotifier);

		m_groupIds = viewPtr->m_groupIds;
		m_nodePositions = viewPtr->m_nodePositions;
		m_removedNodeIds = viewPtr->m_removedNodeIds;

		return true;
	}

	return false;
}


bool CTopologyView::IsEqual(const istd::IChangeable& object) const
{
	const CTopologyView* viewPtr = dynamic_cast<const CTopologyView*>(&object);
	if (viewPtr != nullptr){
		return		(m_groupIds == viewPtr->m_groupIds) &&
					(m_nodePositions == viewPtr->m_nodePositions) &&
					(m_removedNodeIds == viewPtr->m_removedNodeIds);
	}

	return false;
}


istd::IChangeableUniquePtr CTopologyView::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CTopologyView);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return istd::IChangeableUniquePtr();
}


bool CTopologyView::ResetData(CompatibilityMode /*mode*/)
{
	istd::CChangeNotifier changeNotifier(this);
	Q_UNUSED(changeNotifier);

	m_groupIds.clear();
	m_nodePositions.clear();
	m_removedNodeIds.clear();

	return true;
}


} // namespace agentinodata


