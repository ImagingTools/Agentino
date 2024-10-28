#pragma once


// ImtCore includes
#include <imtservergql/CObjectCollectionSubscriberControllerComp.h>


namespace agentinogql
{


class CServiceCollectionSubscriberControllerComp: public imtservergql::CObjectCollectionSubscriberControllerComp
{
public:
	typedef imtservergql::CObjectCollectionSubscriberControllerComp BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionSubscriberControllerComp);
	I_END_COMPONENT;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;
};


} // namespace agentinodata
