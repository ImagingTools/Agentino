// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CAgentLogControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Agents.h>


namespace agentgql
{


QJsonObject CAgentLogControllerComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CClearAgentLogPayload response;

	if (gqlRequest.GetCommandId() != sdl::V1_0::agentino::CClearAgentLogGqlRequest::GetCommandId()){
		errorMessage = QStringLiteral("Unsupported agent log command");
		return QJsonObject();
	}

	sdl::V1_0::agentino::CClearAgentLogGqlRequest clearAgentLogRequest(gqlRequest, true);
	if (!clearAgentLogRequest.IsValid()){
		errorMessage = QStringLiteral("Invalid clear agent log request");
		return QJsonObject();
	}

	sdl::V1_0::agentino::ClearAgentLogRequestArguments arguments = clearAgentLogRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QStringLiteral("Agent id is required to clear the agent log");
		return QJsonObject();
	}

	if (!m_agentLogCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Agent log collection is not configured");
		return QJsonObject();
	}

	response.success = m_agentLogCollectionCompPtr->RemoveElementSet(nullptr);
	if (!response.success){
		errorMessage = QStringLiteral("Unable to clear agent log");
		return QJsonObject();
	}

	QJsonObject responseObject;
	if (!response.WriteToJsonObject(responseObject)){
		errorMessage = QStringLiteral("Unable to create clear agent log response");
		return QJsonObject();
	}

	return responseObject;
}


} // namespace agentgql