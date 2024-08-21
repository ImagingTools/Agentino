#include <agentgql/CAgentMessageCollectionControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#undef GetObject
#undef StartService

// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>
#include <ilog/CMessage.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


bool CAgentMessageCollectionControllerComp::SetupGqlItem(
			const imtgql::CGqlRequest& gqlRequest,
			imtbase::CTreeItemModel& model,
			int itemIndex,
			const imtbase::IObjectCollectionIterator* objectCollectionIterator,
			QString& errorMessage) const
{
	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty() && objectCollectionIterator != nullptr){
		ilog::CMessage* messagePtr = nullptr;
		imtbase::IObjectCollection::DataPtr massageDataPtr;
		if (objectCollectionIterator->GetObjectData(massageDataPtr)){
			messagePtr = dynamic_cast<ilog::CMessage*>(massageDataPtr.GetPtr());
		}

		if (messagePtr != nullptr){
			QByteArray serviceId;
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "Id"){
					serviceId = objectCollectionIterator->GetObjectId();
					elementInformation = serviceId;
				}
				else if(informationId == "Text"){
					elementInformation = messagePtr->GetInformationDescription();
				}
				else if(informationId == "Category"){
					elementInformation = messagePtr->GetInformationCategory();
				}
				else if(informationId == "ID"){
					elementInformation = messagePtr->GetInformationId();
				}
				else if(informationId == "Flags"){
					elementInformation = messagePtr->GetInformationFlags();
				}
				else if(informationId == "Source"){
					elementInformation = messagePtr->GetInformationSource();
				}
				else if(informationId == "Timestamp" || informationId == "LastModified"){
					elementInformation = messagePtr->GetInformationTimeStamp().toString("dd.MM.yyyy hh:mm:ss.zzz");
				}

				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);
			}

			return retVal;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel* CAgentMessageCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CAgentMessageCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CAgentMessageCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


void CAgentMessageCollectionControllerComp::SetObjectFilter(
	const imtgql::CGqlRequest& /*gqlRequest*/,
	const imtbase::CTreeItemModel& objectFilterModel,
	iprm::CParamsSet& filterParams) const
{
	QByteArrayList keys;
	keys << "VerboseFilter" << "InfoFilter" << "WarningFilter" << "ErrorFilter" << "CriticalFilter";
	istd::TDelPtr<iprm::CParamsSet> categoryFilterPtr(new iprm::CParamsSet());

	imtbase::CTreeItemModel *categoryModel = objectFilterModel.GetTreeItemModel("Category");
	if (categoryModel == nullptr){
		keys.clear();
	}

	for (QByteArray key: keys){
		if (categoryModel->ContainsKey(key)){
			QByteArray filterValue = categoryModel->GetData(key).toByteArray();
			if (!filterValue.isEmpty()){
				istd::TDelPtr<iprm::CTextParam> textParamPtr(new iprm::CTextParam());
				textParamPtr->SetText(filterValue);
				categoryFilterPtr->SetEditableParameter(key, textParamPtr.PopPtr());
			}
		}
	}
	filterParams.SetEditableParameter("Category", categoryFilterPtr.PopPtr());

	istd::TDelPtr<iprm::CParamsSet> sourceFilterPtr(new iprm::CParamsSet());

	imtbase::CTreeItemModel *sourceModel = objectFilterModel.GetTreeItemModel("Source");
	if (sourceModel == nullptr){
		return;
	}
	keys.clear();
	for (QString key: sourceModel->GetKeys()){
		keys << key.toUtf8();
	}
	for (QByteArray key: keys){
		if (sourceModel->ContainsKey(key)){
			QByteArray filterValue = sourceModel->GetData(key).toByteArray();
			if (!filterValue.isEmpty()){
				istd::TDelPtr<iprm::CTextParam> textParamPtr(new iprm::CTextParam());
				textParamPtr->SetText(filterValue);
				sourceFilterPtr->SetEditableParameter(key, textParamPtr.PopPtr());
			}
		}
	}
	filterParams.SetEditableParameter("Source", sourceFilterPtr.PopPtr());


}


} // namespace agentgql


