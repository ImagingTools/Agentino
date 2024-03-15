#include <agentgql/CServiceLogControllerComp.h>


// Qt includes
#include <QtCore/QFile>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceLogControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray serviceId;

	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		serviceId = gqlInputParamPtr->GetFieldArgumentValue("Id").toByteArray();
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");

	QString logPath = QString("C:/Users/Public/ImagingTools GmbH/Lisa/LisaServer/LisaServerLog.txt");

	QFile logFile(logPath);
	if (logFile.open(QIODevice::ReadOnly)){
		QByteArray fileData = logFile.readAll();

		dataModelPtr->SetData("Text", fileData);

		logFile.close();
	}

	return rootModelPtr.PopPtr();
}


} // namespace agentgql


