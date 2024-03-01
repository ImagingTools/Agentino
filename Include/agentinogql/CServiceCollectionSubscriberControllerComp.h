#pragma once


// ImtCore includes
#include <imtgql/CObjectCollectionSubscriberControllerComp.h>


namespace agentinogql
{


class CServiceCollectionSubscriberControllerComp: public imtgql::CObjectCollectionSubscriberControllerComp
{
public:
	typedef imtgql::CObjectCollectionSubscriberControllerComp BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionSubscriberControllerComp);
	I_END_COMPONENT;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;
};


} // namespace agentinodata
