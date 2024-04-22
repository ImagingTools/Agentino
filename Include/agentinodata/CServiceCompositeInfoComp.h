#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceCompositeInfo.h>


namespace agentinodata
{


class CServiceCompositeInfoComp:
			public ilog::CLoggerComponentBase,
			virtual public agentinodata::IServiceCompositeInfo
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceCompositeInfoComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceCompositeInfo)
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", true, "ServiceStatusCollection");
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceCompositeInfo)
	virtual QByteArray GetServiceId(const QUrl& url, const QString& connectionServiceTypeName) const override;
	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const override;
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const override;
	virtual StateOfRequiredServices GetStateOfRequiredServices(const QByteArray& serviceId) const override;
	virtual Ids GetDependencyServices(const QByteArray& serviceId) const override;
	virtual QString GetServiceName(const QByteArray& serviceId) const override;
	virtual QString GetServiceAgentName(const QByteArray& serviceId) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
};


} // namespace agentinodata
