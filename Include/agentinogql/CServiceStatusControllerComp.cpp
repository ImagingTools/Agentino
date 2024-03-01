#include <agentinogql/CServiceStatusControllerComp.h>


namespace agentinogql
{


// public methods

// reimplemented (agentinodata::IServiceController)

QProcess::ProcessState CServiceStatusControllerComp::GetServiceStatus(const QByteArray& serviceId) const
{
	return QProcess::ProcessState::NotRunning;
}


bool CServiceStatusControllerComp::StartService(const QByteArray& serviceId)
{
	if (m_requestHandlerCompPtr.IsValid()){
		imtgql::CGqlRequest request(imtgql::CGqlRequest::RT_QUERY, "StartService");
		imtgql::CGqlObject inputObject("input");
		inputObject.InsertField(QByteArray("serviceId"), QVariant(serviceId));
		request.AddParam(inputObject);

		imtgql::CGqlObject additionObject("input");
		QString errorMessage;
		istd::TDelPtr<imtbase::CTreeItemModel> responseModelPtr = m_requestHandlerCompPtr->CreateResponse(request, errorMessage);
		if (responseModelPtr != nullptr){
			if (responseModelPtr->ContainsKey("data")){
				imtbase::CTreeItemModel* dataModelPtr = responseModelPtr->GetTreeItemModel("data");
				if (dataModelPtr != nullptr){
					if (dataModelPtr->ContainsKey("serviceStatus")){
						imtbase::CTreeItemModel* serviceStatusModelPtr = dataModelPtr->GetTreeItemModel("serviceStatus");
						if (serviceStatusModelPtr != nullptr){

						}
					}
				}
			}
		}
	}

	return false;
}


bool CServiceStatusControllerComp::StopService(const QByteArray& serviceId)
{
	if (m_requestHandlerCompPtr.IsValid()){

	}

	return false;
}


} // namespace agentinogql


