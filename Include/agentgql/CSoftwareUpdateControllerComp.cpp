// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CSoftwareUpdateControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>


// Agentino includes
#include <agentinodata/ISoftwareUpdateManager.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)

sdl::V1_0::agentino::CApplyUpdateResponse CSoftwareUpdateControllerComp::OnApplyUpdate(
			const sdl::V1_0::agentino::CApplyUpdateGqlRequest& applyUpdateRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CApplyUpdateResponse response;

	if (!m_updateManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'UpdateManager' was not set", "CSoftwareUpdateControllerComp");

		response.successful = false;
		response.errorMessage = QString("Update manager not available");
		return response;
	}

	sdl::V1_0::agentino::ApplyUpdateRequestArguments arguments = applyUpdateRequest.GetRequestedArguments();

	if (!arguments.input.has_value()){
		response.successful = false;
		response.errorMessage = QString("Invalid request arguments");
		return response;
	}

	if (!arguments.input->updateId.has_value()){
		errorMessage = QString("Update ID is required");
		response.successful = false;
		response.errorMessage = errorMessage;
		return response;
	}

	if (!arguments.input->agentId.has_value()){
		errorMessage = QString("Agent ID is required");
		response.successful = false;
		response.errorMessage = errorMessage;
		return response;
	}

	QByteArray updateId = *arguments.input->updateId;
	QByteArray agentId = *arguments.input->agentId;

	agentinodata::ISoftwareUpdateManager::UpdateResult result = m_updateManagerCompPtr->ApplyUpdate(updateId, agentId);

	response.successful = result.successful;
	response.status = static_cast<sdl::V1_0::agentino::UpdateStatus>(result.status);
	response.errorMessage = result.errorMessage;

	return response;
}


sdl::V1_0::agentino::CRollbackUpdateResponse CSoftwareUpdateControllerComp::OnRollbackUpdate(
			const sdl::V1_0::agentino::CRollbackUpdateGqlRequest& rollbackUpdateRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CRollbackUpdateResponse response;

	if (!m_updateManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'UpdateManager' was not set", "CSoftwareUpdateControllerComp");

		response.successful = false;
		response.errorMessage = QString("Update manager not available");
		return response;
	}

	sdl::V1_0::agentino::RollbackUpdateRequestArguments arguments = rollbackUpdateRequest.GetRequestedArguments();

	if (!arguments.input.has_value()){
		response.successful = false;
		response.errorMessage = QString("Invalid request arguments");
		return response;
	}

	if (!arguments.input->updateId.has_value()){
		errorMessage = QString("Update ID is required");
		response.successful = false;
		response.errorMessage = errorMessage;
		return response;
	}

	if (!arguments.input->agentId.has_value()){
		errorMessage = QString("Agent ID is required");
		response.successful = false;
		response.errorMessage = errorMessage;
		return response;
	}

	QByteArray updateId = *arguments.input->updateId;
	QByteArray agentId = *arguments.input->agentId;

	agentinodata::ISoftwareUpdateManager::UpdateResult result = m_updateManagerCompPtr->RollbackUpdate(updateId, agentId);

	response.successful = result.successful;
	response.status = static_cast<sdl::V1_0::agentino::UpdateStatus>(result.status);
	response.errorMessage = result.errorMessage;

	return response;
}


} // namespace agentgql


