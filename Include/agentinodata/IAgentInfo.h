#pragma once

// Acf includes
#include <iser/IObject.h>

// Qt includes
#include <QtCore/QDateTime>


namespace agentinodata
{


/**
	Interface for describing an agent info.
	\ingroup Agent
*/
class IAgentInfo:
		virtual public iser::IObject
{
public:


//	enum MetaInfoTypes
//	{
//		/**
//			Name given as QString.
//		*/
//		MIT_NAME,

//		/**
//			Service description given as QString.
//		*/
//		MIT_DESCRIPTION,

//		/**
//			Http url to be connection.
//		*/
//		MIT_HTTP_URL,

//		/**
//			Path to settings service given as QString.
//		*/
//		MIT_WEBSOCKET_URL,

//		/**
//			Arguments for start service given as QString.
//		*/
//		MIT_LAST_CONNECTION
//	};


	/**
		Get name of the agent.
	*/
	virtual QString GetAgentName() const = 0;

	/**
		Set name of the agent.
	*/
	virtual void SetAgentName(const QByteArray& agentName) = 0;

	/**
		Get description of the agent.
	*/
	virtual QString GetAgentDescription() const = 0;

	/**
		Set description of the agent.
	*/
	virtual void SetAgentDescription(const QByteArray& agentDescription) = 0;

	/**
		Get http url of the agent.
	*/
	virtual QByteArray GetHttpUrl() const = 0;

	/**
		Set http url of the agent.
	*/
	virtual void SetHttpUrl(const QByteArray& httpUrl) = 0;

	/**
		Get websocket url of the agent.
	*/
	virtual QByteArray GetWebSocketUrl() const = 0;

	/**
		Set http url of the agent.
	*/
	virtual void SetWebSocketUrl(const QByteArray& webSocketUrl) = 0;

	/**
		Get last connection of the agent.
	*/
	virtual QDateTime GetLastConnection() const = 0;
};


} // namespace agentinodata


