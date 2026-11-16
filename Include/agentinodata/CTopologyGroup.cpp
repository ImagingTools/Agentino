// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CTopologyGroup.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/CClassInfo.h>
#include <iser/IArchive.h>
#include <iser/CPrimitiveTypesSerializer.h>


namespace agentinodata
{


// public static methods

QByteArray CTopologyGroup::GetTypeName()
{
	return istd::CClassInfo::GetName<CTopologyGroup>();
}


// public methods

CTopologyGroup::CTopologyGroup()
{
}


void CTopologyGroup::SetMemberIds(const QByteArrayList& memberIds)
{
	if (m_memberIds != memberIds){
		istd::CChangeNotifier changeNotifier(this);
		Q_UNUSED(changeNotifier);

		m_memberIds = memberIds;
	}
}


// reimplemented (iser::IObject)

QByteArray CTopologyGroup::GetFactoryId() const
{
	return GetTypeName();
}


// reimplemented (iser::ISerializable)

bool CTopologyGroup::Serialize(iser::IArchive& archive)
{
	istd::CChangeNotifier changeNotifier(archive.IsStoring()? nullptr: this, &GetAllChanges());
	Q_UNUSED(changeNotifier);

	bool retVal = BaseClass::Serialize(archive);

	retVal = retVal && iser::CPrimitiveTypesSerializer::SerializeContainer(archive, m_memberIds, "Members", "MemberId");

	return retVal;
}


// reimplemented (istd::IChangeable)

int CTopologyGroup::GetSupportedOperations() const
{
	return SO_COPY | SO_CLONE | SO_COMPARE;
}


bool CTopologyGroup::CopyFrom(const IChangeable& object, CompatibilityMode mode)
{
	istd::CChangeNotifier changeNotifier(this);
	Q_UNUSED(changeNotifier);

	// A plain position (layout update from SaveTopology) moves the group and keeps its members.
	const CTopologyGroup* groupPtr = dynamic_cast<const CTopologyGroup*>(&object);
	if (groupPtr != nullptr){
		SetMemberIds(groupPtr->GetMemberIds());
	}

	return BaseClass::CopyFrom(object, mode);
}


bool CTopologyGroup::IsEqual(const istd::IChangeable& object) const
{
	const CTopologyGroup* groupPtr = dynamic_cast<const CTopologyGroup*>(&object);
	if (groupPtr != nullptr){
		return BaseClass::IsEqual(object) && (m_memberIds == groupPtr->m_memberIds);
	}

	return false;
}


istd::IChangeableUniquePtr CTopologyGroup::CloneMe(CompatibilityMode mode) const
{
	istd::IChangeableUniquePtr clonePtr(new CTopologyGroup);
	if (clonePtr->CopyFrom(*this, mode)){
		return clonePtr;
	}

	return istd::IChangeableUniquePtr();
}


} // namespace agentinodata


