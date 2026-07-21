// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CUrlConnectionLinkParamRepresentationController.h>


// ACF includes
#include <iprm/TParamsPtr.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/IServiceManager.h>


namespace agentinodata
{


// protected methods

QUrl CUrlConnectionLinkParamRepresentationController::GetDependantConnectionUrl(
			imtbase::IObjectCollection& agentCollection,
			IServiceManager* serviceManager,
			const QByteArray& dependantId) const
{
	if (serviceManager == nullptr) {
		return QUrl();
	}

	const imtbase::ICollectionInfo::Ids agentIds = agentCollection.GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds) {
		imtbase::IObjectCollection* serviceCollectionPtr = serviceManager->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr) {
			continue;
		}

		const imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceElementId : serviceElementIds) {
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)) {
				continue;
			}
			IServiceInfo* serviceInfoPtr = dynamic_cast<IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr) {
				continue;
			}

			imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
			if (connectionCollectionPtr == nullptr) {
				continue;
			}

			const imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& connectionElementId : connectionElementIds) {
				imtbase::IObjectCollection::DataPtr connectionParamDataPtr;
				if (!connectionCollectionPtr->GetObjectData(connectionElementId, connectionParamDataPtr)) {
					continue;
				}
				imtservice::CUrlConnectionParam* connectionParamPtr =
							dynamic_cast<imtservice::CUrlConnectionParam*>(connectionParamDataPtr.GetPtr());
				if (connectionParamPtr == nullptr) {
					continue;
				}

				imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections =
							connectionParamPtr->GetIncomingConnections();
				for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections) {
					if (incomingConnection.GetObjectUuid() == dependantId) {
						QUrl url;
						if (incomingConnection.GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, url)) {
							return url;
						}
					}
				}
			}
		}
	}

	return QUrl();
}


// public methods

// reimplemented (imtserverapp::IRepresentationController)

QByteArray CUrlConnectionLinkParamRepresentationController::GetModelId() const
{
	return QByteArray();
}


bool CUrlConnectionLinkParamRepresentationController::IsModelSupported(const istd::IChangeable& dataModel) const
{
	const imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(&dataModel);

	return urlConnectionParamPtr != nullptr;
}


bool CUrlConnectionLinkParamRepresentationController::GetRepresentationFromDataModel(
			const istd::IChangeable& dataModel,
			QJsonObject& representation,
			const iprm::IParamsSet* paramsPtr) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	const imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(&dataModel);
	if (urlConnectionParamPtr == nullptr){
		return false;
	}

	QByteArray dependantServiceConnectionId = urlConnectionParamPtr->GetDependantServiceConnectionId();
	QString serviceTypeId = urlConnectionParamPtr->GetServiceTypeId();

	representation.insert(QStringLiteral("DependantConnectionId"), QString::fromUtf8(dependantServiceConnectionId));
	representation.insert(QStringLiteral("ServiceTypeName"), serviceTypeId);

	if (paramsPtr != nullptr){
		iprm::TParamsPtr<imtbase::IObjectCollection> agentCollectionPtr(paramsPtr, "AgentCollection");
		iprm::TParamsPtr<IServiceManager> serviceManagerPtr(paramsPtr, "ServiceManager");
		if (agentCollectionPtr.IsValid()){
			QUrl url = GetDependantConnectionUrl(
						*const_cast<imtbase::IObjectCollection*>(agentCollectionPtr.GetPtr()),
						const_cast<IServiceManager*>(serviceManagerPtr.GetPtr()),
						dependantServiceConnectionId);
			if (url.isValid()){
				representation.insert(QStringLiteral("Url"), url.toString());
			}
		}
	}

	return true;
}


bool CUrlConnectionLinkParamRepresentationController::GetDataModelFromRepresentation(
			const QJsonObject& representation,
			istd::IChangeable& dataModel) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(&dataModel);
	if (urlConnectionParamPtr == nullptr){
		return false;
	}

	if (representation.contains(QStringLiteral("DependantConnectionId"))){
		QByteArray dependantConnectionId = representation.value(QStringLiteral("DependantConnectionId")).toString().toUtf8();

		urlConnectionParamPtr->SetDependantServiceConnectionId(dependantConnectionId);
	}

	return true;
}


} // namespace agentinodata


