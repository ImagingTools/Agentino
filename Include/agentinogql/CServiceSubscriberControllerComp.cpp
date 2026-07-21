// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceSubscriberControllerComp.h>


// ACF includes
#include <istd/TDelPtr.h>

// Agentino includes
#include <agentinodata/CServiceStatusInfo.h>
#include <GeneratedFiles/agentinodata/Ddl/Cpp/Globals.h>


namespace agentinogql
{


namespace
{


QString StatusWireString(agentinodata::IServiceStatusInfo::ServiceStatus status)
{
	// SDL / Topology wire form (RUNNING, NOT_RUNNING, …) — not DDL camelCase and not SS_* enum names.
	switch (status){
	case agentinodata::IServiceStatusInfo::SS_RUNNING:
		return QStringLiteral("RUNNING");
	case agentinodata::IServiceStatusInfo::SS_NOT_RUNNING:
		return QStringLiteral("NOT_RUNNING");
	case agentinodata::IServiceStatusInfo::SS_STARTING:
		return QStringLiteral("STARTING");
	case agentinodata::IServiceStatusInfo::SS_STOPPING:
		return QStringLiteral("STOPPING");
	case agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
		return QStringLiteral("RUNNING_IMPOSSIBLE");
	default:
		return QStringLiteral("UNDEFINED");
	}
}


} // namespace


// protected methods

// reimplemented (icomp::CComponentBase)

void CServiceSubscriberControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();

	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->AttachObserver(this);
	}
	else{
		SendErrorMessage(
					0,
					"Attribute 'Model' is not set — status publish is disabled",
					"CServiceSubscriberControllerComp");
	}
}

void CServiceSubscriberControllerComp::OnComponentDestroyed()
{
	if (m_modelCompPtr.IsValid()){
		m_modelCompPtr->DetachObserver(this);
	}

	BaseClass::OnComponentDestroyed();
}


// reimplemented (imod::CSingleModelObserverBase)

void CServiceSubscriberControllerComp::OnUpdate(const istd::IChangeable::ChangeSet& changeSet)
{
	if (!changeSet.GetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED).isValid()){
		return;
	}

	const agentinodata::IServiceController::NotifierStatusInfo notifierStatusInfo =
				changeSet.GetChangeInfo(agentinodata::IServiceController::CN_STATUS_CHANGED)
							.value<agentinodata::IServiceController::NotifierStatusInfo>();

	const QByteArray serviceId = notifierStatusInfo.serviceId;
	if (serviceId.isEmpty()){
		return;
	}

	// Primary duty: push WS status. Never depends on ServiceStatusCollection.
	PublishStatusPayload(serviceId, notifierStatusInfo.serviceStatus);

	// Secondary (server only): keep optional mirror in sync. Agent leaves this unset by design.
	UpdateOptionalStatusCollection(serviceId, notifierStatusInfo.serviceStatus);
}


// private methods

void CServiceSubscriberControllerComp::PublishStatusPayload(
			const QByteArray& serviceId,
			agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus) const
{
	if (m_commandIdsAttrPtr.GetCount() <= 0){
		SendErrorMessage(
					0,
					"CommandIds is empty — nowhere to publish service status",
					"CServiceSubscriberControllerComp");

		return;
	}

	const QString status = StatusWireString(serviceStatus);
	const QString data = QStringLiteral("{ \"serviceid\": \"%1\", \"serviceStatus\": \"%2\" }")
							   .arg(QString::fromUtf8(serviceId), status);

	const QByteArray payload = data.toUtf8();
	for (int index = 0; index < m_commandIdsAttrPtr.GetCount(); ++index){
		PublishData(m_commandIdsAttrPtr[index], payload);
	}
}


void CServiceSubscriberControllerComp::UpdateOptionalStatusCollection(
			const QByteArray& serviceId,
			agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		// Expected on the agent: no local status repository; ServiceController is the source of truth.
		return;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		agentinodata::CServiceStatusInfo* serviceStatusInfoPtr =
					dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
		if (serviceStatusInfoPtr == nullptr){
			// Miswired collection (e.g. ServiceInfo objects). Do not cast-fail into publish path.
			SendErrorMessage(
						0,
						QString("ServiceStatusCollection entry '%1' is not ServiceStatusInfo — mirror update skipped")
									.arg(QString::fromUtf8(serviceId)),
						"CServiceSubscriberControllerComp");

			return;
		}

		if (serviceStatusInfoPtr->GetServiceStatus() == serviceStatus){
			return;
		}

		serviceStatusInfoPtr->SetServiceStatus(serviceStatus);
		if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr)){
			SendErrorMessage(
						0,
						QString("Unable to set status '%1' for service '%2' in ServiceStatusCollection")
									.arg(StatusWireString(serviceStatus), QString::fromUtf8(serviceId)),
						"CServiceSubscriberControllerComp");
		}

		return;
	}

	istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
	serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);
	serviceStatusInfoPtr->SetServiceId(serviceId);
	serviceStatusInfoPtr->SetServiceStatus(serviceStatus);

	const QByteArray insertedId = m_serviceStatusCollectionCompPtr->InsertNewObject(
				"ServiceStatusInfo",
				"",
				"",
				serviceStatusInfoPtr.PopPtr(),
				serviceId);
	if (insertedId.isEmpty()){
		SendErrorMessage(
					0,
					QString("Unable to insert status '%1' for service '%2' into ServiceStatusCollection")
								.arg(StatusWireString(serviceStatus), QString::fromUtf8(serviceId)),
					"CServiceSubscriberControllerComp");
	}
}


} // namespace agentinogql
