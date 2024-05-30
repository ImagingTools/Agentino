#include <agentinodata/CServiceInfoRepresentationController.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>


namespace agentinodata
{


// reimplemented (imtbase::IRepresentationController)

QByteArray CServiceInfoRepresentationController::GetModelId() const
{
	return QByteArray();
}


bool CServiceInfoRepresentationController::IsModelSupported(const istd::IChangeable& dataModel) const
{
	const CServiceInfo* serviceInfoPtr = dynamic_cast<const CServiceInfo*>(&dataModel);
	if (serviceInfoPtr != nullptr) {
		return true;
	}

	return false;
}


bool CServiceInfoRepresentationController::GetRepresentationFromDataModel(
			const istd::IChangeable& dataModel,
			imtbase::CTreeItemModel& representation,
			const iprm::IParamsSet* paramsPtr) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	CServiceInfo* serviceInfoPtr = dynamic_cast<CServiceInfo*>(const_cast<istd::IChangeable*>(&dataModel));
	if (serviceInfoPtr == nullptr) {
		return false;
	}

	QString servicePath = serviceInfoPtr->GetServicePath();
	QString startScriptPath = serviceInfoPtr->GetStartScriptPath();
	QString stopScriptPath = serviceInfoPtr->GetStopScriptPath();
	QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
	QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');
	bool isAutoStart = serviceInfoPtr->IsAutoStart();
	int tracingLevel = serviceInfoPtr->GetTracingLevel();
	QString serviceTypeName = serviceInfoPtr->GetServiceTypeName();
	QString serviceVersion = serviceInfoPtr->GetServiceVersion();

	representation.SetData("Path", servicePath);
	representation.SetData("StartScript", servicePath);
	representation.SetData("StopScript", servicePath);
	representation.SetData("SettingsPath", settingsPath);
	representation.SetData("Arguments", arguments);
	representation.SetData("IsAutoStart", isAutoStart);
	representation.SetData("TracingLevel", tracingLevel);
	representation.SetData("ServiceTypeName", serviceTypeName);
	representation.SetData("Version", serviceVersion);

	imtbase::CTreeItemModel* inputConnectionsModelPtr = representation.AddTreeModel("InputConnections");
	imtbase::CTreeItemModel* outputConnectionsModelPtr = representation.AddTreeModel("OutputConnections");

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
	if (connectionCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = connectionCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (connectionCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				if (connectionParamPtr != nullptr){
					imtbase::CTreeItemModel representationModel;
					bool ok = m_urlConnectionParamRepresentationController.GetRepresentationFromDataModel(*connectionParamPtr, representationModel, paramsPtr);
					if (ok){
						int index = inputConnectionsModelPtr->InsertNewItem();

						inputConnectionsModelPtr->CopyItemDataFromModel(index, &representationModel);
					}
				}
			}
		}
	}

	imtbase::IObjectCollection* dependantServiceCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
	if (dependantServiceCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids elementIds = dependantServiceCollectionPtr->GetElementIds();
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			if (dependantServiceCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
				imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
				if (connectionLinkParamPtr != nullptr){
					imtbase::CTreeItemModel representationModel;
					bool ok = m_urlConnectionLinkParamRepresentationController.GetRepresentationFromDataModel(*connectionLinkParamPtr, representationModel, paramsPtr);
					if (ok){
						int index = outputConnectionsModelPtr->InsertNewItem();

						outputConnectionsModelPtr->CopyItemDataFromModel(index, &representationModel);
					}
				}
			}
		}
	}

	return true;
}


bool CServiceInfoRepresentationController::GetDataModelFromRepresentation(
			const imtbase::CTreeItemModel& representation,
			istd::IChangeable& dataModel) const
{
	if (!IsModelSupported(dataModel)){
		return false;
	}

	CServiceInfo* serviceInfoPtr = dynamic_cast<CServiceInfo*>(&dataModel);
	if (serviceInfoPtr == nullptr) {
		return false;
	}

	if (representation.ContainsKey("Path")){
		QByteArray path = representation.GetData("Path").toByteArray();

		serviceInfoPtr->SetServicePath(path);
	}

	if (representation.ContainsKey("StartScript")){
		QByteArray path = representation.GetData("StartScript").toByteArray();

		serviceInfoPtr->SetStartScriptPath(path);
	}

	if (representation.ContainsKey("StopScript")){
		QByteArray path = representation.GetData("StopScript").toByteArray();

		serviceInfoPtr->SetStopScriptPath(path);
	}

	if (representation.ContainsKey("SettingsPath")){
		QByteArray settingsPath = representation.GetData("SettingsPath").toByteArray();

		serviceInfoPtr->SetServiceSettingsPath(settingsPath);
	}

	if (representation.ContainsKey("Arguments")){
		QByteArrayList arguments = representation.GetData("Arguments").toByteArray().split(' ');

		serviceInfoPtr->SetServiceArguments(arguments);
	}

	if (representation.ContainsKey("IsAutoStart")){
		bool isAutoStart = representation.GetData("IsAutoStart").toBool();

		serviceInfoPtr->SetIsAutoStart(isAutoStart);
	}

	if (representation.ContainsKey("TracingLevel")){
		int tracingLevel = representation.GetData("TracingLevel").toInt();

		serviceInfoPtr->SetTracingLevel(tracingLevel);
	}

	if (representation.ContainsKey("ServiceTypeName")){
		QByteArray serviceTypeName = representation.GetData("ServiceTypeName").toByteArray();

		serviceInfoPtr->SetServiceTypeName(serviceTypeName);
	}

	if (representation.ContainsKey("Version")){
		QByteArray serviceVersion = representation.GetData("Version").toByteArray();

		serviceInfoPtr->SetServiceVersion(serviceVersion);
	}

	imtbase::IObjectCollection* incomingConnectionCollectionPtr = serviceInfoPtr->GetInputConnections();
	if (incomingConnectionCollectionPtr != nullptr){
		incomingConnectionCollectionPtr->ResetData();

		if (representation.ContainsKey("InputConnections")){
			imtbase::CTreeItemModel* inputConnectionsModelPtr = representation.GetTreeItemModel("InputConnections");
			if (inputConnectionsModelPtr != nullptr){
				for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
					imtbase::CTreeItemModel urlRepresentationModel;
					if (!urlRepresentationModel.CopyItemDataFromModel(0, inputConnectionsModelPtr, i)){
						continue;
					}

					QByteArray id;
					if (urlRepresentationModel.ContainsKey("Id")){
						id = urlRepresentationModel.GetData("Id").toByteArray();
					}

					QString name;
					if (urlRepresentationModel.ContainsKey("ConnectionName")){
						name = urlRepresentationModel.GetData("ConnectionName").toString();
					}

					QString description;
					if (urlRepresentationModel.ContainsKey("Description")){
						description = urlRepresentationModel.GetData("Description").toString();
					}

					istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
					urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam);

					bool ok = m_urlConnectionParamRepresentationController.GetDataModelFromRepresentation(urlRepresentationModel, *urlConnectionParamPtr.GetPtr());
					if (ok){
						incomingConnectionCollectionPtr->InsertNewObject("ConnectionInfo", name, description, urlConnectionParamPtr.PopPtr(), id);
					}
				}
			}
		}
	}

	imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
	if (dependantConnectionCollectionPtr != nullptr){
		dependantConnectionCollectionPtr->ResetData();

		if (representation.ContainsKey("OutputConnections")){
			imtbase::CTreeItemModel* outputConnectionsModelPtr = representation.GetTreeItemModel("OutputConnections");
			if (outputConnectionsModelPtr != nullptr){
				for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
					imtbase::CTreeItemModel urlRepresentationModel;
					if (!urlRepresentationModel.CopyItemDataFromModel(0, outputConnectionsModelPtr, i)){
						continue;
					}

					QByteArray id;
					if (urlRepresentationModel.ContainsKey("Id")){
						id = urlRepresentationModel.GetData("Id").toByteArray();
					}

					QString name;
					if (urlRepresentationModel.ContainsKey("ConnectionName")){
						name = urlRepresentationModel.GetData("ConnectionName").toString();
					}

					QString description;
					if (urlRepresentationModel.ContainsKey("Description")){
						description = urlRepresentationModel.GetData("Description").toString();
					}

					istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
					urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam);

					bool ok = m_urlConnectionLinkParamRepresentationController.GetDataModelFromRepresentation(urlRepresentationModel, *urlConnectionLinkParamPtr.GetPtr());
					if (ok){
						dependantConnectionCollectionPtr->InsertNewObject("ConnectionLink", name, description, urlConnectionLinkParamPtr.PopPtr(), id);
					}
				}
			}
		}
	}

	return true;
}


} // namespace agentinodata


