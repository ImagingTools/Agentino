#pragma once


// ACF includes
#include <iser/IObject.h>


namespace agentinodata
{


/**
	Interface for describing an agent status info.
	\ingroup Agent
*/
class IAgentStatusInfo: virtual public iser::IObject
{
public:
	enum AgentStatus
	{
		AS_UNDEFINED,
		AS_CONNECTED,
		AS_DISCONNECTED
	};

	I_DECLARE_ENUM(AgentStatus, AS_UNDEFINED, AS_CONNECTED, AS_DISCONNECTED);

	/**
		Get ID of the agent.
	*/
	virtual QByteArray GetAgentId() const = 0;

	/**
		Get ID of the agent.
	*/
	virtual AgentStatus GetAgentStatus() const = 0;
};


} // namespace agentinodata


