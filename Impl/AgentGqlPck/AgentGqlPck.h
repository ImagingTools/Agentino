// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <icomp/TModelCompWrap.h>
#include <icomp/TMakeComponentWrap.h>

// Agentino includes
#include <agentgql/TVisualStatusControllerCompWrap.h>
#include <agentgql/TDocumentRevisionControllerCompWrap.h>
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentgql/CServiceControllerComp.h>
#include <agentgql/CAgentinoSubscriptionClientComp.h>
#include <agentgql/CServiceLogControllerComp.h>
#include <agentgql/CMessageCollectionControllerComp.h>
#include <agentgql/CAgentMessageCollectionControllerComp.h>
#include <agentgql/CAgentSettingsControllerComp.h>
#include <agentgql/CServiceLogSubscriberControllerComp.h>
#include <agentgql/CAgentGqlRemoteRepresentationControllerComp.h>
#include <agentgql/CAgentConnectionSubscriberControllerComp.h>
#include <agentgql/CAgentServicesRemoteSubscriberProxyComp.h>


/**
	AgentGqlPck package
*/
namespace AgentGqlPck
{


typedef agentgql::CServiceCollectionControllerComp ServiceCollectionController;
typedef agentgql::CServiceControllerComp ServiceController;
typedef agentgql::CAgentinoSubscriptionClientComp AgentinoSubscriptionClient;
typedef agentgql::CServiceLogControllerComp ServiceLogController;
typedef agentgql::CMessageCollectionControllerComp MessageCollectionController;
typedef agentgql::CAgentMessageCollectionControllerComp AgentMessageCollectionController;
typedef agentgql::CAgentSettingsControllerComp AgentSettingsController;
typedef agentgql::CServiceLogSubscriberControllerComp ServiceLogSubscriberController;
typedef agentgql::CAgentGqlRemoteRepresentationControllerComp AgentGqlRemoteRepresentationController;
typedef agentgql::CAgentConnectionSubscriberControllerComp AgentConnectionSubscriberController;
typedef agentgql::CAgentServicesRemoteSubscriberProxyComp AgentServicesRemoteSubscriberProxy;
typedef agentgql::TVisualStatusControllerCompWrap<AgentGqlRemoteRepresentationController> AgentRemoteVisualStatusController;
typedef agentgql::TDocumentRevisionControllerCompWrap<AgentGqlRemoteRepresentationController> AgentRemoteDocumentRevisionController;


} // namespace AgentGqlPck


