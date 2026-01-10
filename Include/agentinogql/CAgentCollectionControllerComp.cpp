#include <agentinogql/CAgentCollectionControllerComp.h>


// ACF includes
#include <istd/TDelPtr.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/IObjectCollectionIterator.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtgql/CGqlContext.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinogql
{


// reimplemented (icomp::CComponentBase)

void CAgentCollectionControllerComp::OnComponentCreated()
{
	m_timer.setInterval(500);
	m_timer.setSingleShot(true);
	connect(&m_timer, &QTimer::timeout, this, &CAgentCollectionControllerComp::OnTimeout);
}


// reimplemented (sdl::agentino::Agents::CAgentCollectionControllerCompBase)

bool CAgentCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::agentino::Agents::CAgentsListGqlRequest& agentsListRequest,
			sdl::agentino::Agents::CAgentItem::V1_0& representationObject,
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

	sdl::agentino::Agents::AgentsListRequestInfo requestInfo = agentsListRequest.GetRequestInfo();
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
		imtbase::IObjectCollection* serviceCollectionPtr = agentPtr->GetServiceCollection();
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
		QString statusStr = "Disconnected";

		if (m_agentStatusCollectionCompPtr.IsValid()){
			imtbase::IObjectCollection::DataPtr dataPtr;
			if (m_agentStatusCollectionCompPtr->GetObjectData(objectId, dataPtr)){
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

		representationObject.status = statusStr;
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
	const sdl::agentino::Agents::CGetAgentGqlRequest& agentItemRequest,
	sdl::agentino::Agents::CAgentData::V1_0& representationPayload,
	QString& errorMessage) const
{
	const agentinodata::CAgentInfo* agentPtr = dynamic_cast<const agentinodata::CAgentInfo*>(&data);
	if (agentPtr == nullptr){
		errorMessage = QString("Unable to create representation for the agent object. Error: Object is invalid");
		SendErrorMessage(0, errorMessage, "CAgentCollectionControllerComp");

		return false;
	}

	sdl::agentino::Agents::GetAgentRequestArguments arguments = agentItemRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT(false);
		return false;
	}

	QByteArray agentId;
	if (arguments.input.Version_1_0->id){
		agentId = *arguments.input.Version_1_0->id;
	}

	QString name = m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
	QString description = m_objectCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();

	representationPayload.id = agentId;
	representationPayload.name = name;
	representationPayload.description = description;
	representationPayload.lastConnection = agentPtr->GetLastConnection().toString("dd.MM.yyyy");
	representationPayload.tracingLevel = agentPtr->GetTracingLevel();

	return true;
}


bool CAgentCollectionControllerComp::UpdateObjectFromRepresentationRequest(
	const ::imtgql::CGqlRequest& /*rawGqlRequest*/,
	const sdl::agentino::Agents::CUpdateAgentGqlRequest& agentUpdateRequest,
	istd::IChangeable& object,
	QString& errorMessage) const
{
	sdl::agentino::Agents::UpdateAgentRequestArguments inputArguments = agentUpdateRequest.GetRequestedArguments();
	if (!inputArguments.input.Version_1_0){
		I_CRITICAL();
		return false;
	}

	if (!inputArguments.input.Version_1_0->id.has_value()){
		I_CRITICAL();
		return false;
	}

	QByteArray objectId;
	if (inputArguments.input.Version_1_0->id){
		objectId = *inputArguments.input.Version_1_0->id;
	}

	sdl::agentino::Agents::CAgentData::V1_0 agentData;
	if (inputArguments.input.Version_1_0->item){
		agentData = *inputArguments.input.Version_1_0->item;
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
	agentImplPtr.MoveCastedPtr<agentinodata::IAgentInfo>(agentInstancePtr);
	if (!agentImplPtr.IsValid()){
		errorMessage = QT_TR_NOOP("Unable to get an service info!");

		return nullptr;
	}

	imtbase::CTreeItemModel itemModel;
	if (!itemModel.CreateFromJson(itemData)){
		return nullptr;
	}

	agentImplPtr->SetObjectUuid(objectId);

	if (itemModel.ContainsKey("computerName")){
		QString computerName = itemModel.GetData("computerName").toString();
		agentImplPtr->SetComputerName(computerName);
	}

	if (itemModel.ContainsKey("version")){
		QString version = itemModel.GetData("version").toString();
		agentImplPtr->SetVersion(version);
	}

	if (itemModel.ContainsKey("tracingLevel")){
		int tracingLevel = itemModel.GetData("tracingLevel").toInt();
		agentImplPtr->SetTracingLevel(tracingLevel);
	}

	istd::IChangeableUniquePtr retVal;
	retVal.MoveCastedPtr<agentinodata::CIdentifiableAgentInfo>(agentImplPtr);

	return retVal;
}


imtbase::CTreeItemModel* CAgentCollectionControllerComp::InsertObject(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const imtgql::CGqlParamObject& inputParams = gqlRequest.GetParams();

	QByteArray agentId = GetObjectIdFromInputParams(inputParams);

	if (agentId.isEmpty()){
		return nullptr;
	}

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
			return nullptr;
		}

		istd::TUniqueInterfacePtr<agentinodata::CIdentifiableAgentInfo> agentImplPtr;
		agentImplPtr.MoveCastedPtr<istd::IChangeable>(agentInstancePtr);
		if (!agentImplPtr.IsValid()){
			return nullptr;
		}

		QString name = agentImplPtr->GetComputerName();
		agentImplPtr->SetLastConnection(QDateTime::currentDateTimeUtc());
		m_objectCollectionCompPtr->InsertNewObject("DocumentInfo", name, "", agentImplPtr.GetPtr(), agentId);
	}

	m_connectedAgents.append(agentId);
	m_timer.start();

	return nullptr;
}


bool CAgentCollectionControllerComp::UpdateServiceStatusFromAgent(const QByteArray& agentId, const QByteArray& serviceId) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceStatusCollection' was not set", "CAgentCollectionControllerComp");
		return false;
	}

	namespace servicessdl = sdl::agentino::Services;

	servicessdl::GetServiceStatusRequestArguments arguments;
	arguments.input.Version_1_0 = sdl::imtbase::ImtCollection::CInputId::V1_0();
	arguments.input.Version_1_0->id = serviceId;

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

	servicessdl::ServiceStatus status = *response.Version_1_0->status;
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
	while (!m_connectedAgents.isEmpty()){
		QByteArray agentId = m_connectedAgents.at(0);
		m_connectedAgents.removeFirst();

		agentinodata::CAgentInfo* agentInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		}

		if (agentInfoPtr == nullptr){
			SendErrorMessage(0, QString("Unable to update services for agent '%1'. Error: Agent is invalid"), "CAgentCollectionControllerComp");

			return;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		Q_ASSERT(serviceCollectionPtr != nullptr);
		if (agentInfoPtr == nullptr){

			return;
		}

		imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
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
				SendErrorMessage(0, QString("Unable to update service status"), "CAgentCollectionControllerComp");

				return;
			}
		}
	}
}


} // namespace agentinogql


