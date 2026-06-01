// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CLocalTopologyControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase)

sdl::V1_0::agentino::CLocalTopology CLocalTopologyControllerComp::OnGetLocalTopology(
			const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& /*getLocalTopologyRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CLocalTopology response;

	if (!m_localTopologyInfoCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'LocalTopologyInfo' was not set", "CLocalTopologyControllerComp");

		return response;
	}

	const QByteArrayList serviceIds = m_localTopologyInfoCompPtr->GetServiceIds();

	QList<sdl::V1_0::agentino::CLocalServicePosition> positionList;
	positionList.reserve(serviceIds.size());

	for (const QByteArray& serviceId : serviceIds){
		const QPoint point = m_localTopologyInfoCompPtr->GetServicePosition(serviceId);

		sdl::V1_0::agentino::CLocalServicePosition position;
		position.id = serviceId;
		position.x = point.x();
		position.y = point.y();

		positionList << position;
	}

	response.positions.Emplace();
	response.positions->FromList(positionList);

	return response;
}


sdl::V1_0::agentino::CSaveLocalTopologyResponse CLocalTopologyControllerComp::OnSaveLocalTopology(
			const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& saveLocalTopologyRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSaveLocalTopologyResponse response;
	response.successful = false;

	if (!m_localTopologyInfoCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'LocalTopologyInfo' was not set", "CLocalTopologyControllerComp");

		return response;
	}

	sdl::V1_0::agentino::SaveLocalTopologyRequestArguments arguments = saveLocalTopologyRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to save local topology. Error: GraphQL version 1.0 is invalid");
		SendErrorMessage(0, errorMessage, "CLocalTopologyControllerComp");

		return response;
	}

	if (!arguments.input->positions.has_value()){
		response.successful = true;

		return response;
	}

	const istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CLocalServicePosition>> positionList =
			*arguments.input->positions;

	for (const istd::TNullableValue<sdl::V1_0::agentino::CLocalServicePosition>& position : *positionList.GetPtr()){
		if (!position.has_value() || !position->id.has_value()){
			continue;
		}

		const QByteArray serviceId = *position->id;
		const int x = position->x.has_value() ? *position->x : 0;
		const int y = position->y.has_value() ? *position->y : 0;

		if (!m_localTopologyInfoCompPtr->SetServicePosition(serviceId, QPoint(x, y))){
			errorMessage = QString("Unable to save position for service '%1'").arg(QString::fromUtf8(serviceId));
			SendErrorMessage(0, errorMessage, "CLocalTopologyControllerComp");

			return response;
		}
	}

	response.successful = true;

	return response;
}


} // namespace agentgql
