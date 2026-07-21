// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoBase.h>
#include <agentinodata/IServiceStatusProvider.h>


namespace agentinodata
{


/**
	Agent local implementation of the service composite info
	working directly on the agent's own service collection.
	It allows the agent server to resolve the topology of its own services
	independently of the central Agentino server.
	Dependencies to services of other agents cannot be resolved locally
	and are reported as unknown.
	\ingroup Service
*/
class CAgentServiceCompositeInfoComp:
			public ilog::CLoggerComponentBase,
			public CServiceCompositeInfoBase
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CAgentServiceCompositeInfoComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceCompositeInfo)
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Local service collection of the agent", true, "ServiceCollection");
		I_ASSIGN(m_serviceStatusProviderCompPtr, "ServiceStatusProvider", "Provider of the local service status", true, "ServiceController");
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceCompositeInfo)
	virtual QByteArray GetServiceId(const QUrl& url) const override;
	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const override;
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override;
	virtual StateOfRequiredServices GetStateOfRequiredServices(const QByteArray& serviceId) const override;
	virtual Ids GetDependencyServices(const QByteArray& serviceId) const override;
	virtual QString GetServiceName(const QByteArray& serviceId) const override;
	virtual QString GetServiceAgentName(const QByteArray& serviceId) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(agentinodata::IServiceStatusProvider, m_serviceStatusProviderCompPtr);
};


} // namespace agentinodata
