#include <agentgql/CServiceControllerComp.h>


// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceControllerComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_serviceControllerCompPtr.IsValid()){
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
	imtbase::CTreeItemModel* dataModel = nullptr;
	QByteArray serviceId;

	if (paramList.count() > 0 && paramList[0].GetFieldIds().contains("serviceId")){
		serviceId = paramList[0].GetFieldArgumentValue("serviceId").toByteArray();
	}

	if (serviceId.isEmpty()){
		errorMessage = QString("Invalid input parameters, service Id missing.");
	}

	if (!errorMessage.isEmpty()){
		imtbase::CTreeItemModel* notificationItemModel = rootModelPtr->AddTreeModel("errors");
		notificationItemModel->SetData("message", errorMessage);
	}
	else {
		bool result = false;

		if (commandId == "ServiceStart"){
			result = m_serviceControllerCompPtr->StartService(serviceId);
		}
		else if (commandId == "ServiceStop"){
			result = m_serviceControllerCompPtr->StopService(serviceId);
		}

		for (int i = 0; i < count; i++){
			if (fieldList.at(i).GetId() == "serviceStatus"){

				QProcess::ProcessState state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
				agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);

				dataModel = new imtbase::CTreeItemModel();
				imtbase::CTreeItemModel* statusModel = dataModel->AddTreeModel("serviceStatus");

				statusModel->SetData("serviceId", serviceId);
				statusModel->SetData("status", processStateEnum.id);
				statusModel->SetData("statusName", processStateEnum.name);
			}
		}
		rootModelPtr->SetExternTreeModel("data", dataModel);
		return rootModelPtr.PopPtr();
	}
}


} // namespace agentgql


