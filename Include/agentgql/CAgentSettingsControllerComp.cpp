#include <agentgql/CAgentSettingsControllerComp.h>


// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentgql
{


imtbase::CTreeItemModel* CAgentSettingsControllerComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_agentinoUrlCompPtr.IsValid()){
		Q_ASSERT(0);

		return nullptr;
	}

	if (!m_loginCompPtr.IsValid()){
		Q_ASSERT(0);

		return nullptr;
	}

	QByteArray commandId = gqlRequest.GetCommandId();

	if (m_commandIdsAttrPtr.FindValue(commandId) < 0){
		return nullptr;
	}

	const QList<imtgql::CGqlObject> fieldList = gqlRequest.GetFields();
	const QList<imtgql::CGqlObject> paramList = gqlRequest.GetParams();

	int count = fieldList.count();
	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	bool result = false;

	if (commandId == "GetAgentSettings"){
		imtbase::CTreeItemModel* dataModel = rootModelPtr->AddTreeModel("data");
		imtbase::CTreeItemModel* settingsModel = dataModel->AddTreeModel("settings");
		QString agentinoUrl = m_agentinoUrlCompPtr->GetUrl().toString();
		settingsModel->SetData("agentinoUrl", agentinoUrl);
	}
	else if (commandId == "SetAgentSettings"){
		QByteArray inputAgentinoUrl;

		if (paramList.count() > 0 && paramList[0].GetFieldIds().contains("agentinoUrl")){
			inputAgentinoUrl = paramList[0].GetFieldArgumentValue("agentinoUrl").toByteArray();
		}
		if (!inputAgentinoUrl.isEmpty()){
			m_loginCompPtr->Logout();
			if (m_agentinoUrlCompPtr->SetUrl(QUrl(inputAgentinoUrl))){
				imtbase::CTreeItemModel* dataModel = rootModelPtr->AddTreeModel("data");
				imtbase::CTreeItemModel* notificationModel = dataModel->AddTreeModel("updateNotification");
				notificationModel->SetData("successful", true);
			}
			m_loginCompPtr->Login("", "");
		}
	}

	return rootModelPtr.PopPtr();
}


} // namespace agentgql


