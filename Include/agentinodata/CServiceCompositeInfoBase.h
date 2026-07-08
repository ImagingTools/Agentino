// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceCompositeInfo.h>


namespace agentinodata
{


class IServiceInfo;


/**
	Base class providing common service topology resolution logic
	working on a single service collection.
	It is shared between the server side implementation (working on the agent collection)
	and the agent local implementation (working on the agent's own service collection).
	\ingroup Service
*/
class CServiceCompositeInfoBase: virtual public IServiceCompositeInfo
{
protected:
	/**
		Find ID of the service inside of the given service collection
		having an input connection matching the given URL.
		\return ID of the found service or an empty ID, if no service matches.
	*/
	static QByteArray FindServiceIdByUrl(imtbase::IObjectCollection& serviceCollection, const QUrl& url);

	/**
		Find ID of the service inside of the given service collection
		owning the input connection with the given connection ID.
		\return ID of the found service or an empty ID, if no service matches.
	*/
	static QByteArray FindServiceIdByDependantConnectionId(
				imtbase::IObjectCollection& serviceCollection,
				const QByteArray& dependantServiceConnectionId);

	/**
		Calculate the cumulated state of the services the given service depends on.
		Service IDs and states of the dependant services are resolved
		via \c GetServiceId and \c GetServiceStatus of the concrete implementation.
	*/
	StateOfRequiredServices CalculateStateOfRequiredServices(IServiceInfo& serviceInfo) const;

	/**
		Append IDs of the services inside of the given service collection
		depending on the service with the given ID to \c result.
	*/
	void CollectDependencyServices(
				imtbase::IObjectCollection& serviceCollection,
				const QByteArray& serviceId,
				Ids& result) const;
};


} // namespace agentinodata
