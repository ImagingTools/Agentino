#pragma once


// ACF includes
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/CObjectCollection.h>

// ServiceManager includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/IServiceController.h>


namespace agentinodata
{


class CServiceControllerComp:
		public QObject,
		public ilog::CLoggerComponentBase,
		virtual public IServiceController
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceControllerComp);
		I_REGISTER_INTERFACE(agentinodata::IServiceController);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", true, "ServiceCollection");
	I_END_COMPONENT;

	// reimplemented (agentinodata::IServiceController)
	virtual QProcess::ProcessState GetServiceStatus(const QByteArray& serviceId) const override;
	virtual bool StartService(const QByteArray& serviceId) override;
	virtual bool StopService(const QByteArray& serviceId) override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

public	Q_SLOTS:
	void stateChanged(QProcess::ProcessState newState);

private:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);

	QMap<QByteArray, QProcess*> m_processMap;
};


} // namespace agentinodata



