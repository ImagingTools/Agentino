#include <agentgql/CServiceControllerComp.h>


// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceControllerComp::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	QByteArray commandId = gqlRequest.GetCommandId();

	if (m_commandIdsAttrPtr.FindValue(commandId) < 0){
		Q_ASSERT(false);
		return nullptr;
	}

	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
	if (inputParamPtr == nullptr){
		errorMessage = QString("Unable to create response for command '%1'. Error: GraphQL input parameters is invalid").arg(qPrintable(commandId));
		return nullptr;
	}

	QByteArray serviceId = inputParamPtr->GetFieldArgumentValue("serviceid").toByteArray();
	if (serviceId.isEmpty()){
		errorMessage = QString("Unable to create response for command '%1'. Error: Service ID is empty");
		return nullptr;
	}

	if (commandId == "ServiceStart"){
		if (!m_serviceControllerCompPtr->StartService(serviceId)){
			errorMessage = QString("Unable to create response for command '%1'. Error when trying to start the service");
			return nullptr;
		}
	}
	else if (commandId == "ServiceStop"){
		if (!m_serviceControllerCompPtr->StopService(serviceId)){
			errorMessage = QString("Unable to create response for command '%1'. Error when trying to stop the service");
			return nullptr;
		}
	}

	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");

	imtbase::CTreeItemModel* statusModel = dataModelPtr->AddTreeModel("serviceStatus");

	statusModel->SetData("serviceid", serviceId);
	statusModel->SetData("status", processStateEnum.id);
	statusModel->SetData("statusName", processStateEnum.name);

	return rootModelPtr.PopPtr();
}


} // namespace agentgql


