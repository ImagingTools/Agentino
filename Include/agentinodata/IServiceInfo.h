#pragma once


// ACF includes
#include <iser/IObject.h>

// Agentino includes
#include <agentinodata/IServiceMetaInfo.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>


namespace agentinodata
{


/**
	Interface for describing an service info.
	\ingroup Service
*/
class IServiceInfo:
		virtual public iser::IObject
{
public:

	/**
		Supported service types.
	*/
	enum ServiceType
	{
		/**
			A commmon type service.
		*/
		ST_NONE,

		/**
			A plugin based type service.
		*/
		ST_PLUGIN
	};

	/**
		Get type of the service.
	*/
	virtual ServiceType GetServiceType() const = 0;

	/**
		Get path of the service.
	*/
	virtual QByteArray GetServicePath() const = 0;

	/**
		Get settings path of the service.
	*/
	virtual QByteArray GetServiceSettingsPath() const = 0;

	/**
		Get arguments of the service.
	*/
	virtual QByteArrayList GetServiceArguments() const = 0;

	/**
		Get metainfo of the service.
	*/
	virtual const IServiceMetaInfo* GetServiceMetaInfo() const = 0;

	/**
		Get autostart flag.
	*/
	virtual bool IsAutoStart() const = 0;

	/**
		Get connection collection.
	*/
	virtual imtbase::IObjectCollection* GetConnectionCollection() = 0;
};


} // namespace agentinodata


