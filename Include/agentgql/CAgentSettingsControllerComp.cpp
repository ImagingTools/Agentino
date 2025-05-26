#include <agentgql/CAgentSettingsControllerComp.h>


namespace agentgql
{


imtbase::CTreeItemModel* CAgentSettingsControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_agentinoUrlCompPtr.IsValid()){
		Q_ASSERT(0);

		return nullptr;
	}

	if (!m_loginCompPtr.IsValid()){
		Q_ASSERT(0);

		return nullptr;
	}

	const imtgql::CGqlParamObject* gqlInputParamPtr = gqlRequest.GetParamObject("input");
	if (gqlInputParamPtr == nullptr){
		SendErrorMessage(0, QString("GraphQL input parameters is invalid"));

		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	if (gqlRequest.GetRequestType() == imtgql::IGqlRequest::RT_QUERY){
		imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");
		QString agentinoUrl = m_agentinoUrlCompPtr->GetUrl().toString();
		dataModelPtr->SetData("Url", agentinoUrl);
	}
	else if (gqlRequest.GetRequestType() == imtgql::IGqlRequest::RT_MUTATION){
		QByteArray agentinoUrl;

		QByteArray itemData = gqlInputParamPtr->GetFieldArgumentValue("Item").toByteArray();

		imtbase::CTreeItemModel itemModel;
		if (!itemModel.CreateFromJson(itemData)){
			SendErrorMessage(0, QString("Unable to create model from json: '%1'").arg(qPrintable(itemData)));

			return nullptr;
		}

		if (itemModel.ContainsKey("Url")){
			agentinoUrl = itemModel.GetData("Url").toByteArray();
		}

		if (agentinoUrl.isEmpty()){
			errorMessage = QString("URL cannot be empty");

			return nullptr;
		}

		QUrl url(agentinoUrl);

		if (!url.isValid()){
			errorMessage = QString("URL is invalid");

			return nullptr;
		}

		m_loginCompPtr->Disconnect();

		if (m_agentinoUrlCompPtr->SetUrl(url)){
			imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");
			imtbase::CTreeItemModel* notificationModelPtr = dataModelPtr->AddTreeModel("updatedNotification");
			notificationModelPtr->SetData("Id", "");
			notificationModelPtr->SetData("Name", "Agent Settings");
		}

		m_loginCompPtr->Connect();
	}

	return rootModelPtr.PopPtr();
}


} // namespace agentgql


