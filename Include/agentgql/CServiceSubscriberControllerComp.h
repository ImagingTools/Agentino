#pragma once


// ACF includes

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtgql/CGqlSubscriberControllerCompBase.h>


namespace agentgql
{


class CServiceSubscriberControllerComp:
			public imtgql::CGqlSubscriberControllerCompBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef imtgql::CGqlSubscriberControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceSubscriberControllerComp);
		I_ASSIGN(m_modelCompPtr, "Model", "Model", true, "Model");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", true, "ServiceStatusCollection");
	I_END_COMPONENT;

protected:
	virtual bool SetSubscriptions() override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

protected:
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
};


} // namespace agentgql


