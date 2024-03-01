#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtgql/IGqlRequestHandler.h>

// Agentino includes
#include <agentinodata/IServiceController.h>


namespace agentinogql
{


class CServiceStatusControllerComp: public ilog::CLoggerComponentBase, virtual public agentinodata::IServiceController
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceStatusControllerComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceController);
		I_ASSIGN(m_requestHandlerCompPtr, "GqlRequestHandler", "Graphql request handler to create the subscription body", false, "GqlRequestHandler");
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceController)
	virtual QProcess::ProcessState GetServiceStatus(const QByteArray& serviceId) const override;
	virtual bool StartService(const QByteArray& serviceId) override;
	virtual bool StopService(const QByteArray& serviceId) override;

protected:
	I_REF(imtgql::IGqlRequestHandler, m_requestHandlerCompPtr);
};


} // namespace agentinogql


