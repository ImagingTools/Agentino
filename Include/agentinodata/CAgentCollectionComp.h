#pragma once


// ImtCore includes
#include <imtbase/CObjectCollectionComp.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>


namespace agentinodata
{


class CAgentCollectionComp: public imtbase::CObjectCollectionComp, virtual public IServiceManager
{
public:
	typedef imtbase::CObjectCollectionComp BaseClass;

	I_BEGIN_COMPONENT(CAgentCollectionComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceManager);
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceManager)
	virtual bool AddService(
				const QByteArray& agentId,
				const IServiceInfo& serviceInfo,
				const QByteArray& serviceId = QByteArray(),
				const QString& serviceName = QString(),
				const QString& serviceDescription = QString()) override;
	virtual bool RemoveService(const QByteArray& agentId, const QByteArray& serviceId) override;
	virtual bool SetService(const QByteArray& agentId, const QByteArray& serviceId, const IServiceInfo& serviceInfo) override;
	virtual bool ServiceExists(const QByteArray& agentId, const QByteArray& serviceId) const override;
};


} // namespace agentinodata



