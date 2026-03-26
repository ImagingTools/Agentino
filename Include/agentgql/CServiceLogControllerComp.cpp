// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceLogControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


QJsonObject CServiceLogControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		SendErrorMessage(0, QString("'m_serviceCollectionCompPtr' attribute is invalid"));

		return QJsonObject();
	}

	QByteArray serviceId;

	const imtgql::CGqlParamObject* gqlInputParamPtr = gqlRequest.GetParamObject("input");
	if (gqlInputParamPtr != nullptr){
		serviceId = gqlInputParamPtr->GetParamArgumentValue("id").toByteArray();
	}

	agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(dataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		return QJsonObject();
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	QByteArray serviceName = m_serviceCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();

	QFileInfo fileInfo(servicePath);
	QString pluginPath = fileInfo.path() + "/Plugins";

	if (!m_pluginMap.contains(serviceName)){
		istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
		pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

		if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")){
			SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(qPrintable(serviceName)), "CServiceLogControllerComp");
			m_pluginMap.remove(serviceName);

			return QJsonObject();
		}
	}

/// Something unfinished??? It seems, this code doing nothing \todo INSPECT IT AND FIX OR REMOVE!!!!!!!!!!
#if 0
	istd::TDelPtr<imtbase::IObjectCollection> logCollectionPtr;
	if (m_pluginMap.contains(serviceName)){
		for (int index = 0; index < m_pluginMap[serviceName]->m_plugins.count(); index++){
			imtservice::IObjectCollectionPlugin* pluginPtr = m_pluginMap[serviceName]->m_plugins[index].pluginPtr;
			if (pluginPtr != nullptr){
				logCollectionPtr.SetPtr(pluginPtr->GetObjectCollectionFactory()->CreateInstance());
				break;
			}
		}
	}
#endif

	return QJsonObject();
}


} // namespace agentgql


