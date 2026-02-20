// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// Qt includes
#include <QtCore/qglobal.h>

// ImtCore includes
#include <imtlic/CProductInfo.h>
#include <imtlic/CFeatureInfo.h>


namespace agentino
{


static void FillProduct(imtlic::IProductInfo& productInfo){
	productInfo.SetProductId("Agentino");
	productInfo.SetName(QT_TRANSLATE_NOOP("Product", "Agentino"));
	productInfo.SetCategoryId("Software");

	istd::TDelPtr<imtlic::CIdentifiableFeatureInfo> agentinoManagementFeatureInfo;
	agentinoManagementFeatureInfo.SetPtr(new imtlic::CIdentifiableFeatureInfo);
	agentinoManagementFeatureInfo->SetObjectUuid("ff13ac00-e869-4467-828c-ba4bc3fafa53");
	agentinoManagementFeatureInfo->SetFeatureId("AgentinoManagement");
	agentinoManagementFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Agentino Management"));
	agentinoManagementFeatureInfo->SetOptional(false);
	agentinoManagementFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> serviceManagementFeatureInfo;
	serviceManagementFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	serviceManagementFeatureInfo->SetFeatureId("ServiceManagement");
	serviceManagementFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Service Management"));
	serviceManagementFeatureInfo->SetOptional(false);
	serviceManagementFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> stopServiceFeatureInfo;
	stopServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	stopServiceFeatureInfo->SetFeatureId("StopService");
	stopServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Stop Service"));
	stopServiceFeatureInfo->SetOptional(false);
	stopServiceFeatureInfo->SetIsPermission(true);

	serviceManagementFeatureInfo->InsertSubFeature(stopServiceFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> startServiceFeatureInfo;
	startServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	startServiceFeatureInfo->SetFeatureId("StartService");
	startServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Start Service"));
	startServiceFeatureInfo->SetOptional(false);
	startServiceFeatureInfo->SetIsPermission(true);

	serviceManagementFeatureInfo->InsertSubFeature(startServiceFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> viewServicesFeatureInfo;
	viewServicesFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	viewServicesFeatureInfo->SetFeatureId("ViewServices");
	viewServicesFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "View Services"));
	viewServicesFeatureInfo->SetOptional(false);
	viewServicesFeatureInfo->SetIsPermission(true);

	serviceManagementFeatureInfo->InsertSubFeature(viewServicesFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> editServiceFeatureInfo;
	editServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	editServiceFeatureInfo->SetFeatureId("EditService");
	editServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Edit Service"));
	editServiceFeatureInfo->SetOptional(false);
	editServiceFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> changeServiceFeatureInfo;
	changeServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	changeServiceFeatureInfo->SetFeatureId("ChangeService");
	changeServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Change Service"));
	changeServiceFeatureInfo->SetOptional(false);
	changeServiceFeatureInfo->SetIsPermission(true);

	editServiceFeatureInfo->InsertSubFeature(changeServiceFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> removeServiceFeatureInfo;
	removeServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	removeServiceFeatureInfo->SetFeatureId("RemoveService");
	removeServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Remove Service"));
	removeServiceFeatureInfo->SetOptional(false);
	removeServiceFeatureInfo->SetIsPermission(true);

	editServiceFeatureInfo->InsertSubFeature(removeServiceFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> addServiceFeatureInfo;
	addServiceFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	addServiceFeatureInfo->SetFeatureId("AddService");
	addServiceFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Add Service"));
	addServiceFeatureInfo->SetOptional(false);
	addServiceFeatureInfo->SetIsPermission(true);

	editServiceFeatureInfo->InsertSubFeature(addServiceFeatureInfo.PopPtr());

	serviceManagementFeatureInfo->InsertSubFeature(editServiceFeatureInfo.PopPtr());

	agentinoManagementFeatureInfo->InsertSubFeature(serviceManagementFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> agentManagementFeatureInfo;
	agentManagementFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	agentManagementFeatureInfo->SetFeatureId("AgentManagement");
	agentManagementFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Agent Management"));
	agentManagementFeatureInfo->SetOptional(false);
	agentManagementFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> editAgentFeatureInfo;
	editAgentFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	editAgentFeatureInfo->SetFeatureId("EditAgent");
	editAgentFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Edit Agent"));
	editAgentFeatureInfo->SetOptional(false);
	editAgentFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> removeAgentFeatureInfo;
	removeAgentFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	removeAgentFeatureInfo->SetFeatureId("RemoveAgent");
	removeAgentFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Remove Agent"));
	removeAgentFeatureInfo->SetOptional(false);
	removeAgentFeatureInfo->SetIsPermission(true);

	editAgentFeatureInfo->InsertSubFeature(removeAgentFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> addAgentFeatureInfo;
	addAgentFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	addAgentFeatureInfo->SetFeatureId("AddAgent");
	addAgentFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Add Agent"));
	addAgentFeatureInfo->SetOptional(false);
	addAgentFeatureInfo->SetIsPermission(true);

	editAgentFeatureInfo->InsertSubFeature(addAgentFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> changeAgentFeatureInfo;
	changeAgentFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	changeAgentFeatureInfo->SetFeatureId("ChangeAgent");
	changeAgentFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Change Agent"));
	changeAgentFeatureInfo->SetOptional(false);
	changeAgentFeatureInfo->SetIsPermission(true);

	editAgentFeatureInfo->InsertSubFeature(changeAgentFeatureInfo.PopPtr());

	agentManagementFeatureInfo->InsertSubFeature(editAgentFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> viewAgentsFeatureInfo;
	viewAgentsFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	viewAgentsFeatureInfo->SetFeatureId("ViewAgents");
	viewAgentsFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "View Agents"));
	viewAgentsFeatureInfo->SetOptional(false);
	viewAgentsFeatureInfo->SetIsPermission(true);

	agentManagementFeatureInfo->InsertSubFeature(viewAgentsFeatureInfo.PopPtr());

	agentinoManagementFeatureInfo->InsertSubFeature(agentManagementFeatureInfo.PopPtr());

	istd::TDelPtr<imtlic::CFeatureInfo> topologyManagementFeatureInfo;
	topologyManagementFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	topologyManagementFeatureInfo->SetFeatureId("TopologyManagement");
	topologyManagementFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "Topology Management"));
	topologyManagementFeatureInfo->SetOptional(false);
	topologyManagementFeatureInfo->SetIsPermission(true);

	istd::TDelPtr<imtlic::CFeatureInfo> viewTopologyFeatureInfo;
	viewTopologyFeatureInfo.SetPtr(new imtlic::CFeatureInfo);
	viewTopologyFeatureInfo->SetFeatureId("ViewTopology");
	viewTopologyFeatureInfo->SetFeatureName(QT_TRANSLATE_NOOP("Feature", "View Topology"));
	viewTopologyFeatureInfo->SetOptional(false);
	viewTopologyFeatureInfo->SetIsPermission(true);

	topologyManagementFeatureInfo->InsertSubFeature(viewTopologyFeatureInfo.PopPtr());

	agentinoManagementFeatureInfo->InsertSubFeature(topologyManagementFeatureInfo.PopPtr());

	productInfo.AddFeature("ff13ac00-e869-4467-828c-ba4bc3fafa53", *agentinoManagementFeatureInfo.GetPtr());

}


};

