// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CAgentServiceManagerComp.h>


// ACF includes
#include <istd/CChangeNotifier.h>
#include <istd/TInterfacePtr.h>


namespace agentinodata
{


namespace
{


void InitServiceCollection(imod::TModelWrap<imtbase::CObjectCollection>& collection)
{
	typedef istd::TSingleFactory<istd::IChangeable, agentinodata::CIdentifiableServiceInfo> FactoryServiceImpl;
	collection.RegisterFactory<FactoryServiceImpl>("ServiceInfo");
}


} // namespace


// protected

void CAgentServiceManagerComp::OnComponentDestroyed()
{
	const QList<QByteArray> keys = m_mirrors.keys();
	for (const QByteArray& key : keys) {
		EvictAgent(key);
	}
	BaseClass::OnComponentDestroyed();
}


// private

bool CAgentServiceManagerComp::AgentExists(const QByteArray& agentId) const
{
	if (!m_agentCollectionCompPtr.IsValid() || agentId.isEmpty()) {
		return false;
	}
	return m_agentCollectionCompPtr->GetElementIds().contains(agentId);
}


imtbase::IObjectCollection* CAgentServiceManagerComp::EnsureServiceCollection(const QByteArray& agentId) const
{
	if (agentId.isEmpty() || !AgentExists(agentId)) {
		return nullptr;
	}
	if (!m_mirrors.contains(agentId) || m_mirrors[agentId].collection == nullptr) {
		auto* coll = new imod::TModelWrap<imtbase::CObjectCollection>();
		InitServiceCollection(*coll);
		Mirror mirror;
		mirror.collection = coll;
		m_mirrors.insert(agentId, mirror);
	}
	return m_mirrors[agentId].collection;
}


imtbase::IObjectCollection* CAgentServiceManagerComp::GetServiceCollection(const QByteArray& agentId) const
{
	return EnsureServiceCollection(agentId);
}


void CAgentServiceManagerComp::EvictAgent(const QByteArray& agentId)
{
	if (!m_mirrors.contains(agentId)) {
		return;
	}
	Mirror mirror = m_mirrors.take(agentId);
	if (mirror.collection != nullptr) {
		mirror.collection->ResetData();
		delete mirror.collection;
	}
}


bool CAgentServiceManagerComp::AddService(
			const QByteArray& agentId,
			const IServiceInfo& serviceInfo,
			const QByteArray& serviceId,
			const QString& serviceName,
			const QString& serviceDescription)
{
	imtbase::IObjectCollection* serviceCollectionPtr = EnsureServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr) {
		return false;
	}
	const QByteArray objectId = serviceCollectionPtr->InsertNewObject(
				"ServiceInfo",
				serviceName,
				serviceDescription,
				&serviceInfo,
				serviceId);
	if (objectId.isEmpty()) {
		return false;
	}
	ChangeSet changeSet(CF_SERVICE_ADDED);
	changeSet.SetChangeInfo("agentid", agentId);
	changeSet.SetChangeInfo("serviceid", objectId);
	istd::CChangeNotifier changeNotifier(this, &changeSet);
	return true;
}


bool CAgentServiceManagerComp::RemoveServices(
			const QByteArray& agentId,
			const imtbase::ICollectionInfo::Ids& serviceIds)
{
	imtbase::IObjectCollection* serviceCollectionPtr = EnsureServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr) {
		return false;
	}
	const bool result = serviceCollectionPtr->RemoveElements(serviceIds);
	if (result) {
		for (const QByteArray& id : serviceIds) {
			ChangeSet changeSet(CF_SERVICE_REMOVED);
			changeSet.SetChangeInfo("agentid", agentId);
			changeSet.SetChangeInfo("serviceid", id);
			istd::CChangeNotifier changeNotifier(this, &changeSet);
		}
	}
	return result;
}


bool CAgentServiceManagerComp::SetService(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			const IServiceInfo& serviceInfo,
			const QString& serviceName,
			const QString& serviceDescription,
			bool beQuiet)
{
	imtbase::IObjectCollection* serviceCollectionPtr = EnsureServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr) {
		return false;
	}
	if (!serviceCollectionPtr->SetObjectData(serviceId, serviceInfo)) {
		return false;
	}
	serviceCollectionPtr->SetElementName(serviceId, serviceName);
	serviceCollectionPtr->SetElementDescription(serviceId, serviceDescription);
	if (!beQuiet) {
		ChangeSet changeSet(CF_SERVICE_UPDATED);
		changeSet.SetChangeInfo("agentid", agentId);
		changeSet.SetChangeInfo("serviceid", serviceId);
		istd::CChangeNotifier changeNotifier(this, &changeSet);
	}
	return true;
}


bool CAgentServiceManagerComp::ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const
{
	imtbase::IObjectCollection* serviceCollectionPtr = EnsureServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr) {
		return false;
	}
	return serviceCollectionPtr->GetElementIds().contains(serviceId);
}


IServiceInfo* CAgentServiceManagerComp::GetService(const QByteArray& agentId, const QByteArray& serviceId) const
{
	imtbase::IObjectCollection* serviceCollectionPtr = EnsureServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr) {
		return nullptr;
	}
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!serviceCollectionPtr->GetObjectData(serviceId, dataPtr)) {
		return nullptr;
	}
	istd::TUniqueInterfacePtr<IServiceInfo> serviceInfoPtr;
	if (!serviceInfoPtr.MoveCastedPtr(dataPtr->CloneMe())) {
		return nullptr;
	}
	return serviceInfoPtr.PopInterfacePtr();
}


} // namespace agentinodata
