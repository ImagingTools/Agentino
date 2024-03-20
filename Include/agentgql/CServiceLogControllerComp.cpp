#include <agentgql/CServiceLogControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QLibrary>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


bool CServiceLogControllerComp::LoadPluginDirectory(const QString& pluginDirectoryPath, const QString& serviceName) const
{
	SendInfoMessage(0, QString("Looking for the document plugins in '%1'").arg(pluginDirectoryPath));

	if (!pluginDirectoryPath.isEmpty() && QFileInfo(pluginDirectoryPath).exists()){
		QDir pluginsDirectory(pluginDirectoryPath);

		QFileInfoList pluginsList = pluginsDirectory.entryInfoList(QStringList() << "*.plugin");

		for (const QFileInfo& pluginPath : pluginsList){
#ifdef Q_OS_WIN
			SetDllDirectory(pluginPath.absolutePath().toStdWString().c_str());
#endif
			SendInfoMessage(0, QString("Load: '%1'").arg(pluginPath.canonicalFilePath()));

			QLibrary library(pluginPath.canonicalFilePath());
			if (library.load()){
				IMT_CREATE_PLUGIN_FUNCTION(ServiceLog) createPluginFunc = (IMT_CREATE_PLUGIN_FUNCTION(ServiceLog))library.resolve(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog));
				if (createPluginFunc != NULL){
					istd::TDelPtr<imtservice::IObjectCollectionPlugin> pluginInstancePtr = createPluginFunc();
					if (pluginInstancePtr.IsValid()){
						PluginInfo pluginInfo;
						pluginInfo.pluginPath = pluginPath.canonicalFilePath();
						pluginInfo.pluginPtr = pluginInstancePtr.PopPtr();
						pluginInfo.destroyFunc = (IMT_DESTROY_PLUGIN_FUNCTION(ServiceLog))library.resolve(IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog));
						m_pluginMap.insert(serviceName, pluginInfo);
					}
				}
			}
			else{
				SendErrorMessage(0, QString("%1").arg(library.errorString()));
			}
		}

		return true;
	}

	return false;
}


imtbase::CTreeItemModel* CServiceLogControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		SendErrorMessage(0, QString("'m_serviceCollectionCompPtr' attribute is invalid"));

		return nullptr;
	}

	QByteArray serviceId;

	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		serviceId = gqlInputParamPtr->GetFieldArgumentValue("Id").toByteArray();
	}

	agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(dataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		return nullptr;
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	QString serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toString();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");

	QFileInfo fileInfo(servicePath);
	QString pluginPath = fileInfo.path() + "/Plugins";

	if (!m_pluginMap.contains(serviceName)){
		if (!LoadPluginDirectory(pluginPath, serviceName)){
			SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(serviceName), "CServiceCollectionControllerComp");

			return nullptr;
		}
	}

	istd::TDelPtr<imtbase::IObjectCollection> logCollectionPtr;
	if (m_pluginMap.contains(serviceName)){
		logCollectionPtr.SetPtr(m_pluginMap[serviceName].pluginPtr->GetObjectCollectionFactory()->CreateInstance());
	}

//	QString logPath = QString("C:/Users/Public/ImagingTools GmbH/Lisa/LisaServer/LisaServerLog.txt");

//	QFile logFile(logPath);
//	if (logFile.open(QIODevice::ReadOnly)){
//		QByteArray fileData = logFile.readAll();

//		dataModelPtr->SetData("Text", fileData);

//		logFile.close();
//	}

	return rootModelPtr.PopPtr();
}


} // namespace agentgql


