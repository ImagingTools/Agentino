#pragma once


// ACF includes

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtgql/CObjectCollectionSubscriberControllerComp.h>


namespace agentinogql
{


class CServiceStatusCollectionSubscriberControllerComp: public imtgql::CObjectCollectionSubscriberControllerComp
{
public:
	typedef imtgql::CObjectCollectionSubscriberControllerComp BaseClass;

	I_BEGIN_COMPONENT(CServiceStatusCollectionSubscriberControllerComp);
	I_END_COMPONENT;

protected:
	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;
};


} // namespace agentinogql


