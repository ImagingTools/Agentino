#pragma once

// Acf includes
#include <iser/IObject.h>

// ServiceManager includes
#include <agentinodata/IServiceMetaInfo.h>


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
			A Acf based type service.
		*/
		ST_ACF
	};

	/**
		Get type of the service.
	*/
	virtual ServiceType GetServiceType() const = 0;

	/**
		Get name of the service.
	*/
	virtual QString GetServiceName() const = 0;

	/**
		Get description of the service.
	*/
	virtual QString GetServiceDescription() const = 0;

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

};


} // namespace agentinodata


