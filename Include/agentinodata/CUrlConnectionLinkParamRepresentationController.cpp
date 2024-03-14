#include <agentinodata/CUrlConnectionLinkParamRepresentationController.h>


// ACF includes
#include <iprm/TParamsPtr.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


// protected methods

QUrl CUrlConnectionLinkParamRepresentationController::GetDependantConnectionUrl(
			imtbase::IObjectCollection& objectCollection,
			const QByteArray& dependantId) const
{
	imtbase::ICollectionInfo::Ids agentIds = objectCollection.GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId: agentIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (objectCollection.GetObjectData(agentId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}

					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						imtbase::IObjectCollection::DataPtr connectionParamDataPtr;
						if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionParamDataPtr)){
							imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionParamDataPtr.GetPtr());
							if (connectionParamPtr == nullptr){
								break;
							}

							if (connectionElementId == dependantId){
								return connectionParamPtr->GetUrl();
							}

							QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
							for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
								if (incomingConnection.id == dependantId){
									return incomingConnection.url;
								}
							}
						}
					}
				}
			}
		}
	}

	return QUrl();
}


// public methods

// reimplemented (imtbase::IRepresentationController)

QByteArray CUrlConnectionLinkParamRepresentationController::GetModelId() const
{
	return QByteArray();
}


bool CUrlConnectionLinkParamRepresentationController::IsModelSupported(const istd::IChangeable& dataModel) const
{
	const imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(&dataModel);
	if (urlConnectionParamPtr != nullptr) {
		return true;
	}

	return false;
}


bool CUrlConnectionLinkParamRepresentationController::GetRepresentationFromDataModel(
			const istd::IChangeable& dataModel,
			imtbase::CTreeItemModel& representation,
			const iprm::IParamsSet* paramsPtr) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	const imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(&dataModel);
	if (urlConnectionParamPtr == nullptr) {
		return false;
	}

	QByteArray dependantServiceConnectionId = urlConnectionParamPtr->GetDependantServiceConnectionId();
	QString serviceTypeName = urlConnectionParamPtr->GetServiceTypeName();
	QByteArray usageId = urlConnectionParamPtr->GetUsageId();

	representation.SetData("Id", usageId);
	representation.SetData("UsageId", usageId);
	representation.SetData("DependantConnectionId", dependantServiceConnectionId);
	representation.SetData("ServiceTypeName", serviceTypeName);

	if (paramsPtr != nullptr){
		iprm::TParamsPtr<imtbase::IObjectCollection> agentCollectionPtr(paramsPtr, "AgentCollection");
		if (agentCollectionPtr.IsValid()){
			QUrl url = GetDependantConnectionUrl(*const_cast<imtbase::IObjectCollection*>(agentCollectionPtr.GetPtr()), dependantServiceConnectionId);

			representation.SetData("Url", url.toString());
		}
	}

	return true;
}


bool CUrlConnectionLinkParamRepresentationController::GetDataModelFromRepresentation(
			const imtbase::CTreeItemModel& representation,
			istd::IChangeable& dataModel) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(&dataModel);
	if (urlConnectionParamPtr == nullptr) {
		return false;
	}

	if (representation.ContainsKey("UsageId")){
		QByteArray usageId = representation.GetData("UsageId").toByteArray();

		urlConnectionParamPtr->SetUsageId(usageId);
	}

	if (representation.ContainsKey("ServiceTypeName")){
		QByteArray serviceTypeName = representation.GetData("ServiceTypeName").toByteArray();

		urlConnectionParamPtr->SetServiceTypeName(serviceTypeName);
	}

	if (representation.ContainsKey("DependantConnectionId")){
		QByteArray dependantConnectionId = representation.GetData("DependantConnectionId").toByteArray();

		urlConnectionParamPtr->SetDependantServiceConnectionId(dependantConnectionId);
	}

	return true;
}


} // namespace agentinodata


