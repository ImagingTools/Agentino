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
	QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
	QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');
	bool isAutoStart = serviceInfoPtr->IsAutoStart();
	QString serviceTypeName = serviceInfoPtr->GetServiceTypeName();

	representation.SetData("Path", servicePath);
	representation.SetData("SettingsPath", settingsPath);
	representation.SetData("Arguments", arguments);
	representation.SetData("IsAutoStart", isAutoStart);
	representation.SetData("ServiceTypeName", serviceTypeName);

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
		const imtbase::CTreeItemModel& /*representation*/,
		istd::IChangeable& /*dataModel*/) const
{
	return false;
}


} // namespace agentinodata


