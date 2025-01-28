#pragma once


// ACF includes
#include <imod/TSingleModelObserverBase.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtservergql/CGqlPublisherCompBase.h>


namespace agentinogql
{


class CServiceSubscriberControllerComp:
			public imtservergql::CGqlPublisherCompBase,
			public imod::TSingleModelObserverBase<istd::IChangeable>
{
public:
	typedef imtservergql::CGqlPublisherCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceSubscriberControllerComp);
		I_ASSIGN(m_modelCompPtr, "Model", "Model", true, "Model");
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", false, "ServiceStatusCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

protected:
	I_REF(imod::IModel, m_modelCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
};


} // namespace agentinogql


