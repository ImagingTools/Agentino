#pragma once


// Qt includes
#include <QtCore/QDateTime>

// ACF includes
#include <iser/IObject.h>
#include <ilog/ITracingConfiguration.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>


namespace agentinodata
{


/**
	Interface for describing an agent info.
	\ingroup Agent
*/
class IAgentInfo: virtual public iser::IObject, virtual public ilog::ITracingConfiguration
{
public:
	/**
		Get version of the agent.
	*/
	virtual QString GetVersion() const = 0;

	/**
		Get last connection of the agent.
	*/
	virtual QDateTime GetLastConnection() const = 0;

	/**
		Get computer name.
	*/
	virtual QString GetComputerName() const = 0;

	/**
		Get service collection.
	*/
	virtual imtbase::IObjectCollection* GetServiceCollection() = 0;
};


} // namespace agentinodata


