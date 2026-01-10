#include <agentgql/CAgentMessageCollectionControllerComp.h>


// ACF includes
#include <ilog/CMessage.h>


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
			for (const QByteArray& informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "id"){
					elementInformation = objectCollectionIterator->GetObjectId();
				}
				else if(informationId == "text"){
					elementInformation = messagePtr->GetInformationDescription();
				}
				else if(informationId == "category"){
					elementInformation = messagePtr->GetInformationCategory();
				}
				else if(informationId == "infoId"){
					elementInformation = messagePtr->GetInformationId();
				}
				else if(informationId == "flags"){
					elementInformation = messagePtr->GetInformationFlags();
				}
				else if(informationId == "source"){
					elementInformation = messagePtr->GetInformationSource();
				}
				else if(informationId == "timestamp"){
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


} // namespace agentgql


