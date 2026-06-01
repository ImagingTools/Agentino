// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology_fwd.h>


namespace agentinogql
{


/**
	Interface for querying and pushing local topology data to remote agents.
	Implementations forward GetLocalTopology/SaveLocalTopology GQL requests
	to the targeted agent using the standard clientid-based routing.
*/
class IAgentLocalTopologyProxy: virtual public istd::IChangeable
{
public:
	/**
		Query the local topology from a remote agent.
		On failure (agent offline, timeout, etc.) errorMessage is set and an empty topology is returned.
	*/
	virtual sdl::V1_0::agentino::CLocalTopology QueryLocalTopology(
				const QByteArray& agentId,
				QString& errorMessage) const = 0;

	/**
		Push a local topology to a remote agent so the agent can persist it.
		Returns true if the agent confirmed successful storage.
	*/
	virtual bool PushLocalTopology(
				const QByteArray& agentId,
				const sdl::V1_0::agentino::CLocalTopology& topology,
				QString& errorMessage) const = 0;
};


} // namespace agentinogql
