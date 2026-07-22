// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CAgentCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>
#include <QtCore/QHash>
#include <algorithm>

// ACF includes
#include <istd/TDelPtr.h>
#include <iprm/CTextParam.h>
#include <iprm/CParamsSet.h>

// ImtCore includes
#include <imtbase/IObjectCollectionIterator.h>
#include <imtbase/IComplexCollectionFilter.h>
#include <imtbase/CComplexCollectionFilterHelper.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>


namespace agentinogql
{


// reimplemented (icomp::CComponentBase)

void CAgentCollectionControllerComp::OnComponentCreated()
{
	BaseClass::OnComponentCreated();
	// Affinity of this QObject is the main/GUI thread. Only start m_timer from that thread
	// (see InsertObject hop) so OnTimeout can reconcile agent service mirrors.
	m_timer.setInterval(500);
	m_timer.setSingleShot(true);
	connect(&m_timer, &QTimer::timeout, this, &CAgentCollectionControllerComp::OnTimeout);
}


// reimplemented (sdl::V1_0::agentino::CAgentCollectionControllerCompBase)

bool CAgentCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::V1_0::agentino::CAgentsListGqlRequest& agentsListRequest,
			sdl::V1_0::agentino::CAgentItem& representationObject,
			QString& /*errorMessage*/) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CDeviceCollectionControllerComp");
		return false;
	}

	QByteArray objectId = objectCollectionIterator.GetObjectId();

	agentinodata::CIdentifiableAgentInfo* agentPtr = nullptr;
	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (objectCollectionIterator.GetObjectData(agentDataPtr)){
		agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentDataPtr.GetPtr());
	}

	sdl::V1_0::agentino::AgentsListRequestInfo requestInfo = agentsListRequest.GetRequestInfo();
	if (requestInfo.items.isIdRequested){
		representationObject.id = objectId;
	}

	if (requestInfo.items.isTypeIdRequested){
		QByteArray typeId = m_objectCollectionCompPtr->GetObjectTypeId(objectId);
		representationObject.typeId = typeId;
	}

	if (requestInfo.items.isNameRequested){
		QString name = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();
		representationObject.name = name;
	}

	if (requestInfo.items.isDescriptionRequested){
		QString description = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
		representationObject.description = description;
	}

	if (requestInfo.items.isComputerNameRequested){
		QString computerName = agentPtr->GetComputerName();
		representationObject.computerName = computerName;
	}

	if (requestInfo.items.isServicesRequested){
		// The agent id is the object id from the representation context.
		imtbase::IObjectCollection* serviceCollectionPtr =
					m_serviceManagerCompPtr.IsValid()
								? m_serviceManagerCompPtr->GetServiceCollection(objectId)
								: nullptr;
		if (serviceCollectionPtr != nullptr){
			imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
			QStringList result;
			for (const imtbase::ICollectionInfo::Id& id : ids){
				imtbase::IObjectCollection::DataPtr dataPtr;
				if (serviceCollectionPtr->GetObjectData(id, dataPtr)){
					const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(dataPtr.GetPtr());
					if (serviceInfoPtr != nullptr){
						QString name = serviceInfoPtr->GetServiceName();
						result << name;
					}
				}
			}

			representationObject.services = result.join(';');
		}
	}

	if (requestInfo.items.isStatusRequested){
		representationObject.status = ComputeAgentStatus(objectId);
	}

	if (requestInfo.items.isEnrollmentStatusRequested){
		QString enrollmentStatusStr = EnrollmentStatusToString(EnrollmentStatus::Approved);
		if (m_enrollmentControllerCompPtr.IsValid()){
			enrollmentStatusStr = EnrollmentStatusToString(m_enrollmentControllerCompPtr->Get(objectId).status);
		}
		representationObject.enrollmentStatus = enrollmentStatusStr;
	}

	if (requestInfo.items.isVersionRequested){
		representationObject.version = agentPtr->GetVersion();
	}

	if (requestInfo.items.isLastConnectionRequested){
		QDateTime lastConnection = agentPtr->GetLastConnection().toUTC();
		if (!lastConnection.isNull()){
			representationObject.lastConnection = lastConnection.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
		}
	}

	if (requestInfo.items.isAddedRequested){
		QDateTime addedTime = objectCollectionIterator.GetElementInfo("Added").toDateTime().toUTC();
		QString added = addedTime.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
		representationObject.added = added;
	}

	return true;
}


bool CAgentCollectionControllerComp::CreateRepresentationFromObject(
	const istd::IChangeable& data,
	const sdl::V1_0::agentino::CGetAgentGqlRequest& agentItemRequest,
	sdl::V1_0::agentino::CAgentData& representationPayload,
	QString& errorMessage) const
{
	const agentinodata::CAgentInfo* agentPtr = dynamic_cast<const agentinodata::CAgentInfo*>(&data);
	if (agentPtr == nullptr){
		errorMessage = QString("Unable to create representation for the agent object. Error: Object is invalid");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return false;
	}

	sdl::V1_0::agentino::GetAgentRequestArguments arguments = agentItemRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT(false);
		return false;
	}

	QByteArray agentId;
	if (arguments.input->id){
		agentId = *arguments.input->id;
	}

	if (!IsAgentApproved(agentId)){
		errorMessage = QStringLiteral("Agent '%1' is not approved yet").arg(QString::fromUtf8(agentId));
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return false;
	}

	QString name = m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
	QString description = m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();

	representationPayload.id = agentId;
	representationPayload.name = name;
	representationPayload.description = description;
	representationPayload.lastConnection = agentPtr->GetLastConnection().toString("dd.MM.yyyy");
	representationPayload.tracingLevel = agentPtr->GetTracingLevel();

	if (m_serviceManagerCompPtr.IsValid()){
		imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr != nullptr){
			representationPayload.serviceCount = serviceCollectionPtr->GetElementIds().count();
		}
	}

	return true;
}


QString CAgentCollectionControllerComp::ComputeAgentStatus(const QByteArray& agentId) const
{
	QString statusStr = "Disconnected";

	if (m_agentStatusCollectionCompPtr.IsValid()){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_agentStatusCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			const agentinodata::CAgentStatusInfo* agentStatusInfoPtr = dynamic_cast<const agentinodata::CAgentStatusInfo*>(dataPtr.GetPtr());
			if (agentStatusInfoPtr != nullptr){
				agentinodata::IAgentStatusInfo::AgentStatus status = agentStatusInfoPtr->GetAgentStatus();
				switch(status){
				case agentinodata::IAgentStatusInfo::AS_CONNECTED:
					statusStr = "Connected";
					break;
				case agentinodata::IAgentStatusInfo::AS_DISCONNECTED:
					statusStr = "Disconnected";
					break;
				default:
					statusStr = "Undefined";
				}
			}
		}
	}

	// Non-approved enrollment states override connection status (Pending / Rejected / …) so
	// the Agents list needs no separate Approval column.
	if (m_enrollmentControllerCompPtr.IsValid()){
		const EnrollmentStatus enrollmentStatus = m_enrollmentControllerCompPtr->Get(agentId).status;
		if (enrollmentStatus != EnrollmentStatus::Approved){
			statusStr = QString::fromUtf8(EnrollmentStatusToString(enrollmentStatus));
		}
	}

	return statusStr;
}


QVariant CAgentCollectionControllerComp::GetAgentSortValue(const QByteArray& agentId, const QByteArray& fieldId) const
{
	if (fieldId == "name"){
		return m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
	}
	if (fieldId == "description"){
		return m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
	}
	if (fieldId == "status"){
		return ComputeAgentStatus(agentId);
	}
	if (fieldId == "services"){
		imtbase::IObjectCollection* serviceCollectionPtr =
					m_serviceManagerCompPtr.IsValid()
								? m_serviceManagerCompPtr->GetServiceCollection(agentId)
								: nullptr;
		return serviceCollectionPtr != nullptr ? serviceCollectionPtr->GetElementIds().count() : 0;
	}

	agentinodata::CIdentifiableAgentInfo* agentPtr = nullptr;
	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
		agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentDataPtr.GetPtr());
	}
	if (agentPtr == nullptr){
		return QVariant();
	}

	if (fieldId == "computerName"){
		return agentPtr->GetComputerName();
	}
	if (fieldId == "version"){
		return agentPtr->GetVersion();
	}
	if (fieldId == "lastConnection"){
		return agentPtr->GetLastConnection();
	}

	return QVariant();
}


bool CAgentCollectionControllerComp::UpdateObjectFromRepresentationRequest(
	const ::imtgql::CGqlRequest& /*rawGqlRequest*/,
	const sdl::V1_0::agentino::CUpdateAgentGqlRequest& agentUpdateRequest,
	istd::IChangeable& object,
	QString& errorMessage) const
{
	sdl::V1_0::agentino::UpdateAgentRequestArguments inputArguments = agentUpdateRequest.GetRequestedArguments();
	if (!inputArguments.input){
		I_CRITICAL();
		return false;
	}

	if (!inputArguments.input->id.has_value()){
		I_CRITICAL();
		return false;
	}

	QByteArray objectId;
	if (inputArguments.input->id){
		objectId = *inputArguments.input->id;
	}

	if (!IsAgentApproved(objectId)){
		errorMessage = QStringLiteral("Agent '%1' is not approved yet").arg(QString::fromUtf8(objectId));
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return false;
	}

	sdl::V1_0::agentino::CAgentData agentData;
	if (inputArguments.input->item){
		agentData = *inputArguments.input->item;
	}

	agentinodata::CAgentInfo* agentPtr = dynamic_cast<agentinodata::CAgentInfo*>(&object);
	if (agentPtr == nullptr){
		errorMessage = QString("Unable to create representation for the agent object. Error: Object is invalid");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return false;
	}

	if (agentData.name){
		m_objectCollectionCompPtr->SetElementName(objectId, *agentData.name);
	}

	if (agentData.description){
		m_objectCollectionCompPtr->SetElementDescription(objectId, *agentData.description);
	}

	if (agentData.tracingLevel){
		agentPtr->SetTracingLevel(*agentData.tracingLevel);
	}

	return true;
}


// reimplemented (imtgql::CObjectCollectionControllerCompBase)

istd::IChangeableUniquePtr CAgentCollectionControllerComp::CreateObjectFromRequest(
			const imtgql::CGqlRequest& gqlRequest,
			QByteArray& objectId,
			QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const imtgql::CGqlParamObject* inputDataPtr = gqlRequest.GetParamObject("input");
	if (inputDataPtr == nullptr){
		Q_ASSERT(false);

		return nullptr;
	}

	objectId = inputDataPtr->GetParamArgumentValue("id").toByteArray();
	if (objectId.isEmpty()){
		objectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}

	QByteArray itemData = inputDataPtr->GetParamArgumentValue("item").toByteArray();
	if (itemData.isEmpty()){
		return nullptr;
	}

	istd::TUniqueInterfacePtr<agentinodata::IAgentInfo> agentInstancePtr = m_agentFactCompPtr.CreateInstance();
	if (!agentInstancePtr.IsValid()){
		errorMessage = QT_TR_NOOP("Unable to get an service info!");

		return nullptr;
	}

	istd::TUniqueInterfacePtr<agentinodata::CIdentifiableAgentInfo> agentImplPtr;
	agentImplPtr.MoveCastedPtr<agentinodata::IAgentInfo>(std::move(agentInstancePtr));
	if (!agentImplPtr.IsValid()){
		errorMessage = QT_TR_NOOP("Unable to get an service info!");

		return nullptr;
	}

	QJsonDocument itemDoc = QJsonDocument::fromJson(itemData);
	if (!itemDoc.isObject()){
		return nullptr;
	}

	QJsonObject itemObj = itemDoc.object();

	agentImplPtr->SetObjectUuid(objectId);

	if (itemObj.contains(QStringLiteral("computerName"))){
		QString computerName = itemObj.value(QStringLiteral("computerName")).toString();
		agentImplPtr->SetComputerName(computerName);
	}

	if (itemObj.contains(QStringLiteral("version"))){
		QString version = itemObj.value(QStringLiteral("version")).toString();
		agentImplPtr->SetVersion(version);
	}

	if (itemObj.contains(QStringLiteral("tracingLevel"))){
		int tracingLevel = itemObj.value(QStringLiteral("tracingLevel")).toInt();
		agentImplPtr->SetTracingLevel(tracingLevel);
	}

	istd::IChangeableUniquePtr retVal;
	retVal.MoveCastedPtr<agentinodata::CIdentifiableAgentInfo>(std::move(agentImplPtr));

	return retVal;
}


QJsonObject CAgentCollectionControllerComp::InsertObject(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return QJsonObject();
	}

	const imtgql::CGqlParamObject& inputParams = gqlRequest.GetParams();

	QByteArray agentId = GetObjectIdFromInputParams(inputParams);

	if (agentId.isEmpty()){
		return QJsonObject();
	}

	const GateDecision decision = AdmitAgentFromRequest(gqlRequest, agentId);
	if (decision == GateDecision::Deny){
		// Still fall through to the row upsert below: the agent itself is told it was denied
		// via this errorMessage, but the operator still needs to see/reset it on the Agents
		// page (single unified page for every agent, whatever its enrollment status).
		errorMessage = QStringLiteral("Agent enrollment denied (Rejected/Revoked)");
	}

	const bool approved = (decision == GateDecision::Active);

	// Every agent that contacts the server gets/keeps a row here regardless of its enrollment
	// decision (Pending/Suspended/Rejected/Revoked included) - the Agents page is the single
	// place operators see and act on every agent, not just admitted ones. Only the reconcile/
	// mirror bookkeeping further down stays gated on `approved`.
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CIdentifiableAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			agentInfoPtr->SetLastConnection(QDateTime::currentDateTimeUtc());

			QByteArray item = inputParams.GetParamArgumentValue("item").toByteArray();
			QJsonDocument itemDoc = QJsonDocument::fromJson(item);
			if (itemDoc.object().contains("version")){
				QString version = itemDoc.object().value("version").toString();
				agentInfoPtr->SetVersion(version);
			}

			if (!m_objectCollectionCompPtr->SetObjectData(agentId, *agentInfoPtr)){
				qDebug() << QString("Unable to set data to the collection object with ID: '%1'.").arg(qPrintable(agentId));
			}
		}
	}
	else{
		istd::IChangeableUniquePtr agentInstancePtr = CreateObjectFromRequest(gqlRequest, agentId, errorMessage);
		if (!agentInstancePtr.IsValid()){
			return QJsonObject();
		}

		istd::TUniqueInterfacePtr<agentinodata::CIdentifiableAgentInfo> agentImplPtr;
		agentImplPtr.MoveCastedPtr<istd::IChangeable>(std::move(agentInstancePtr));
		if (!agentImplPtr.IsValid()){
			return QJsonObject();
		}

		QString name = agentImplPtr->GetComputerName();
		agentImplPtr->SetLastConnection(QDateTime::currentDateTimeUtc());
		m_objectCollectionCompPtr->InsertNewObject("AgentInfo", name, "", agentImplPtr.GetPtr(), agentId);
	}

	// InsertObject runs on a GQL CWorkerThread. m_timer / m_connectedAgents / m_approvedAgents
	// belong to this component's thread (main). Starting QTimer from the worker was a no-op /
	// undefined — OnTimeout never drained, so SyncAgentServicesInMirror never ran and
	// Topology/Agents.services stayed empty after connect.
	CAgentCollectionControllerComp* self = const_cast<CAgentCollectionControllerComp*>(this);
	const QByteArray agentIdCopy = agentId;
	const bool queued = QMetaObject::invokeMethod(
				self,
				[self, agentIdCopy, approved]() {
					if (approved){
						self->m_approvedAgents.insert(agentIdCopy);
						// Immediate mirror reconcile (do not depend only on the 500ms timer).
						// Timer still runs for status-bootstrap of already-mirrored services.
						if (self->m_serviceSynchronizerCompPtr.IsValid()){
							QString reconcileError;
							if (!self->m_serviceSynchronizerCompPtr->SyncAgentServicesInMirror(
										agentIdCopy, reconcileError)){
								self->SendErrorMessage(
											0,
											QString("Unable to reconcile services of agent '%1'. Error: %2")
														.arg(QString::fromUtf8(agentIdCopy), reconcileError),
											"CAgentCollectionControllerComp");
							}
						}
						if (!self->m_connectedAgents.contains(agentIdCopy)){
							self->m_connectedAgents.append(agentIdCopy);
						}
						self->m_timer.start();
					}
					else{
						self->m_approvedAgents.remove(agentIdCopy);
					}
				},
				Qt::QueuedConnection);
	if (!queued){
		// Same-thread fallback (unit tests / no event loop yet).
		if (approved){
			m_approvedAgents.insert(agentId);
			if (m_serviceSynchronizerCompPtr.IsValid()){
				QString reconcileError;
				m_serviceSynchronizerCompPtr->SyncAgentServicesInMirror(agentId, reconcileError);
			}
			if (!m_connectedAgents.contains(agentId)){
				m_connectedAgents.append(agentId);
			}
			m_timer.start();
		}
		else{
			m_approvedAgents.remove(agentId);
		}
	}

	return QJsonObject();
}


QJsonObject CAgentCollectionControllerComp::ListObjects(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to list objects. Component reference 'ObjectCollection' was not set");
		SendCriticalMessage(0, errorMessage);

		return QJsonObject();
	}

	QByteArray requestedStatus;
	const imtgql::CGqlParamObject* inputParamsPtr = gqlRequest.GetParamObject("input");
	if (inputParamsPtr != nullptr){
		requestedStatus = inputParamsPtr->GetParamArgumentValue("status").toByteArray();
	}

	// Header-click sorting (Table -> collectionFilter -> filterModel) - the in-memory
	// ObjectCollection this component sits on (imtbase::CObjectCollectionComp, wired in
	// Repositories.acc) never applies filterModel/sorting itself (CObjectCollectionBase::
	// CreateObjectCollectionIterator ignores its selectionParamsPtr/offset/count entirely -
	// unlike DB-backed collections that sort at the query level), so this reimplementation
	// sorts explicitly. PrepareFilters is fed from whichever of "viewParams"/"selectionParams"
	// is actually present - RemoteCollectionView's generic CollectionRepresentation sends
	// "selectionParams" (CreateSubCollectionInput), not the "viewParams" shape AgentListInput
	// itself declares, so only checking "viewParams" would silently never see the sort/filter
	// the client actually sent - same dual check as the base ListObjects (CObjectCollectionControllerCompBase.cpp).
	QByteArray sortFieldId;
	imtbase::IComplexCollectionFilter::SortingOrder sortingOrder = imtbase::IComplexCollectionFilter::SO_NO_ORDER;
	iprm::CParamsSet filterParams;
	if (inputParamsPtr != nullptr){
		const imtgql::CGqlParamObject* viewParamsPtr = inputParamsPtr->GetParamArgumentObjectPtr("viewParams");
		if (viewParamsPtr != nullptr){
			PrepareFilters(gqlRequest, *viewParamsPtr, filterParams);
		}
		else if (inputParamsPtr->ContainsParam("selectionParams")){
			const imtgql::CGqlParamObject* selectionParamsPtr = inputParamsPtr->GetParamArgumentObjectPtr("selectionParams");
			if (selectionParamsPtr != nullptr){
				PrepareFilters(gqlRequest, *selectionParamsPtr, filterParams);
			}
		}

		imtbase::IComplexCollectionFilter* complexFilterPtr =
					dynamic_cast<imtbase::IComplexCollectionFilter*>(filterParams.GetEditableParameter("ComplexFilter"));
		if (complexFilterPtr != nullptr){
			const QSet<QByteArray> sortFieldIds = imtbase::CComplexCollectionFilterHelper::GetSortingFieldIds(*complexFilterPtr);
			if (!sortFieldIds.isEmpty()){
				sortFieldId = *sortFieldIds.constBegin();
				sortingOrder = imtbase::CComplexCollectionFilterHelper::GetSortingOrder(*complexFilterPtr, {sortFieldId});
			}
		}
	}

	istd::TDelPtr<imtbase::IObjectCollectionIterator> objectCollectionIterator(
		m_objectCollectionCompPtr->CreateObjectCollectionIterator(QByteArray(), 0, -1, &filterParams));
	if (objectCollectionIterator == nullptr){
		errorMessage = QString("Object collection iterator creation failed");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return QJsonObject();
	}

	const auto setupContext = CreateGqlItemSetupContext(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return QJsonObject();
	}

	int countAll = 0;
	int countPending = 0;
	int countApproved = 0;
	int countSuspended = 0;
	int countRejected = 0;
	int countRevoked = 0;

	// Item JSON is built while the iterator is positioned at each element (SetupGqlItemWithContext
	// needs a live IObjectCollectionIterator*, and this in-memory collection has no way to seek
	// to an arbitrary id afterwards) - sorting then just reorders matchedAgentIds and the final
	// array is assembled from this map, instead of re-iterating per sorted position.
	QHash<QByteArray, QJsonObject> itemsByAgentId;
	QByteArrayList matchedAgentIds;

	while (objectCollectionIterator->Next()){
		const QByteArray agentId = objectCollectionIterator->GetObjectId();
		const QString status = ComputeAgentStatus(agentId);

		countAll++;
		if (status == "Pending"){
			countPending++;
		}
		else if (status == "Suspended"){
			countSuspended++;
		}
		else if (status == "Rejected"){
			countRejected++;
		}
		else if (status == "Revoked"){
			countRevoked++;
		}
		else{
			// Connected/Disconnected/Undefined all mean Approved (see ComputeAgentStatus).
			countApproved++;
		}

		if (!requestedStatus.isEmpty()){
			bool matches = false;
			if (requestedStatus == "Approved"){
				matches = (status == "Connected" || status == "Disconnected" || status == "Undefined");
			}
			else{
				matches = (QString::fromUtf8(requestedStatus) == status);
			}
			if (!matches){
				continue;
			}
		}

		QJsonObject itemObj;
		if (!SetupGqlItemWithContext(gqlRequest, setupContext, itemObj, objectCollectionIterator.GetPtr(), errorMessage)){
			SendWarningMessage(0, errorMessage, "CAgentCollectionControllerComp");
		}
		itemsByAgentId.insert(agentId, itemObj);
		matchedAgentIds.append(agentId);
	}

	if (!sortFieldId.isEmpty() && sortingOrder != imtbase::IComplexCollectionFilter::SO_NO_ORDER){
		const bool ascending = (sortingOrder == imtbase::IComplexCollectionFilter::SO_ASC);
		std::sort(matchedAgentIds.begin(), matchedAgentIds.end(),
					[this, &sortFieldId, ascending](const QByteArray& agentIdA, const QByteArray& agentIdB){
			int cmp = 0;
			if (sortFieldId == "lastConnection"){
				const QDateTime valueA = GetAgentSortValue(agentIdA, sortFieldId).toDateTime();
				const QDateTime valueB = GetAgentSortValue(agentIdB, sortFieldId).toDateTime();
				cmp = (valueA < valueB) ? -1 : (valueA > valueB ? 1 : 0);
			}
			else if (sortFieldId == "services"){
				const int valueA = GetAgentSortValue(agentIdA, sortFieldId).toInt();
				const int valueB = GetAgentSortValue(agentIdB, sortFieldId).toInt();
				cmp = (valueA < valueB) ? -1 : (valueA > valueB ? 1 : 0);
			}
			else{
				cmp = GetAgentSortValue(agentIdA, sortFieldId).toString()
							.localeAwareCompare(GetAgentSortValue(agentIdB, sortFieldId).toString());
			}
			return ascending ? (cmp < 0) : (cmp > 0);
		});
	}

	QJsonArray itemsArray;
	for (const QByteArray& agentId : matchedAgentIds){
		itemsArray.append(itemsByAgentId.value(agentId));
	}

	QJsonObject notificationObj;
	notificationObj.insert(QStringLiteral("pagesCount"), 1);
	notificationObj.insert(QStringLiteral("totalCount"), itemsArray.count());

	QJsonObject countsObj;
	countsObj.insert(QStringLiteral("all"), countAll);
	countsObj.insert(QStringLiteral("pending"), countPending);
	countsObj.insert(QStringLiteral("approved"), countApproved);
	countsObj.insert(QStringLiteral("suspended"), countSuspended);
	countsObj.insert(QStringLiteral("rejected"), countRejected);
	countsObj.insert(QStringLiteral("revoked"), countRevoked);

	QJsonObject dataObj;
	dataObj.insert(QStringLiteral("notification"), notificationObj);
	dataObj.insert(QStringLiteral("counts"), countsObj);
	dataObj.insert(QStringLiteral("items"), itemsArray);

	QJsonObject rootObj;
	rootObj.insert(QStringLiteral("data"), dataObj);
	return rootObj;
}


GateDecision CAgentCollectionControllerComp::AdmitAgentFromRequest(
			const imtgql::CGqlRequest& gqlRequest,
			const QByteArray& agentId) const
{
	if (!m_enrollmentGateCompPtr.IsValid()){
		// No gate configured → legacy auto-join (dev / transition).
		return GateDecision::Active;
	}

	const imtgql::CGqlParamObject* inputDataPtr = gqlRequest.GetParamObject("input");
	QJsonObject itemObj;
	if (inputDataPtr != nullptr){
		const QByteArray itemData = inputDataPtr->GetParamArgumentValue("item").toByteArray();
		itemObj = QJsonDocument::fromJson(itemData).object();
	}

	IEnrollmentGate::AgentIdentity identity;
	identity.agentId = agentId;

	IEnrollmentGate::AgentAttributes attrs;
	attrs.computerName = itemObj.value(QStringLiteral("computerName")).toString();
	attrs.agentVersion = itemObj.value(QStringLiteral("version")).toString();
	attrs.os = itemObj.value(QStringLiteral("os")).toString();
	attrs.advertisedEndpoint = itemObj.value(QStringLiteral("webSocketUrl")).toString();
	attrs.claimedName = itemObj.value(QStringLiteral("name")).toString();

	EnrollmentRecord record;
	const GateDecision decision = m_enrollmentGateCompPtr->Admit(identity, attrs, record);

	// A denied agent never reaches the approvable list, so the operator has no UI signal
	// at all — log why it was turned away.
	if (decision == GateDecision::Deny){
		SendErrorMessage(
					0,
					QString("Enrollment denied for agent '%1' (status '%2'). Reset or approve it "
								"on the Agents page to let it join again.")
								.arg(QString::fromUtf8(agentId),
									QString::fromUtf8(EnrollmentStatusToString(record.status))),
					"CAgentCollectionControllerComp");
	}

	return decision;
}


bool CAgentCollectionControllerComp::IsAgentApproved(const QByteArray& agentId) const
{
	if (!m_enrollmentControllerCompPtr.IsValid()){
		// No gate configured → legacy auto-join (dev / transition), mirrors AdmitAgentFromRequest.
		return true;
	}

	return m_enrollmentControllerCompPtr->Get(agentId).status == EnrollmentStatus::Approved;
}


bool CAgentCollectionControllerComp::UpdateServiceStatusFromAgent(const QByteArray& agentId, const QByteArray& serviceId) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceStatusCollection' was not set", "CAgentCollectionControllerComp");
		return false;
	}

	namespace servicessdl = sdl::V1_0::agentino;

	servicessdl::GetServiceStatusRequestArguments arguments;
	arguments.input = sdl::V1_0::imtbase::CInputId();
	arguments.input->id = serviceId;

	imtgql::CGqlRequest gqlRequest;

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!servicessdl::CGetServiceStatusGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		SendErrorMessage(0, QString("Unable to update status for service '%1'. Error: Setup GraphQL request failed").arg(qPrintable(serviceId)), "CAgentCollectionControllerComp");

		return false;
	}

	QString errorMessage;
	servicessdl::CServiceStatusResponse response = SendModelRequest<servicessdl::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, QString("Unable to update status for service '%1'. Error: %2").arg(qPrintable(serviceId), errorMessage), "CAgentCollectionControllerComp");

		return false;
	}

	istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
	serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

	serviceStatusInfoPtr->SetServiceId(serviceId);

	servicessdl::ServiceStatus status = *response.status;
	switch (status){
	case servicessdl::ServiceStatus::NOT_RUNNING:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
		break;
	case servicessdl::ServiceStatus::RUNNING:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_RUNNING);
		break;
	case servicessdl::ServiceStatus::STARTING:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_STARTING);
		break;
	case servicessdl::ServiceStatus::STOPPING:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_STOPPING);
		break;
	case servicessdl::ServiceStatus::RUNNING_IMPOSSIBLE:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE);
		break;
	case servicessdl::ServiceStatus::UNDEFINED:
		serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_UNDEFINED);
		break;
	default:
		return false;
	}

	QStringList statuses = serviceStatusInfoPtr->ServiceStatusGetStrings();

	QByteArrayList serviceIds = m_serviceStatusCollectionCompPtr->GetElementIds();
	if (serviceIds.contains(serviceId)){
		if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr.PopPtr())){
			SendErrorMessage(0,
							 QString("Unable to set status '%1' for service '%2'. Error: Set object to status collection failed")
								 .arg(statuses[serviceStatusInfoPtr->GetServiceStatus()], qPrintable(serviceId)),
							 "CAgentCollectionControllerComp");

			return false;
		}
	}
	else{
		QByteArray retVal = m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
		if (retVal.isEmpty()){
			SendErrorMessage(0,
							 QString("Unable to set status '%1' for service '%2'. Error: Insert to status collection failed")
								 .arg(statuses[serviceStatusInfoPtr->GetServiceStatus()], qPrintable(serviceId)),
							 "CAgentCollectionControllerComp");

			return false;
		}
	}

	return true;
}


void CAgentCollectionControllerComp::OnTimeout()
{
	// Prevent re-entrancy while SendModelRequest pumps the local event loop.
	// New AgentAdd entries stay in m_connectedAgents and are drained by this loop
	// or by a subsequent single-shot timer start after we leave.
	if (m_timeoutRunning){
		return;
	}
	m_timeoutRunning = true;

	while (!m_connectedAgents.isEmpty()){
		QByteArray agentId = m_connectedAgents.takeFirst();

		// Enrollment quarantine: never pull domain data for non-approved agents.
		if (!m_approvedAgents.contains(agentId) && m_enrollmentGateCompPtr.IsValid()){
			continue;
		}

		// Full service-set reconciliation once on (re)connect.
		// Live status thereafter comes from OnAgentServiceStatusChanged;
		// skip per-service status polling here to reduce nested SendModelRequest loops (P3/P6).
		if (m_serviceSynchronizerCompPtr.IsValid()){
			QString reconcileError;
			if (!m_serviceSynchronizerCompPtr->SyncAgentServicesInMirror(agentId, reconcileError)){
				SendErrorMessage(
							0,
							QString("Unable to reconcile services of agent '%1'. Error: %2")
										.arg(QString::fromUtf8(agentId), reconcileError),
							"CAgentCollectionControllerComp");
			}
		}

		// Optional one-shot status bootstrap only when status collection is empty for this agent.
		agentinodata::CAgentInfo* agentInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		}

		if (agentInfoPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr =
					m_serviceManagerCompPtr.IsValid()
								? m_serviceManagerCompPtr->GetServiceCollection(agentId)
								: nullptr;
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
		bool needStatusBootstrap = false;
		if (m_serviceStatusCollectionCompPtr.IsValid()){
			for (const imtbase::ICollectionInfo::Id& id : ids){
				imtbase::IObjectCollection::DataPtr st;
				if (!m_serviceStatusCollectionCompPtr->GetObjectData(id, st)){
					needStatusBootstrap = true;
					break;
				}
			}
		}
		if (!needStatusBootstrap){
			continue;
		}

		for (const imtbase::ICollectionInfo::Id& id : ids){
			agentinodata::CServiceInfo* serviceInfoInfoPtr = nullptr;
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (serviceCollectionPtr->GetObjectData(id, serviceDataPtr)){
				serviceInfoInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
			}

			if (serviceInfoInfoPtr == nullptr){
				continue;
			}

			if (!UpdateServiceStatusFromAgent(agentId, id)){
				SendErrorMessage(
							0,
							QString("Unable to update service status for service '%1' of agent '%2'")
										.arg(QString::fromUtf8(id), QString::fromUtf8(agentId)),
							"CAgentCollectionControllerComp");
				// Continue: one failed status must not abort the rest of the queue.
			}
		}
	}

	m_timeoutRunning = false;
}


} // namespace agentinogql


