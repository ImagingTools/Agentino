#pragma once


// ACF includes

// ImtCore includes
// #include <imtbase/IObjectCollection.h>
#include <imtgql/CObjectCollectionSubscriberControllerComp.h>

// Agentino includes
#include <agentinodata/CServiceCompositeInfoComp.h>


namespace agentinogql
{


class CServiceStatusCollectionSubscriberControllerComp: public imtgql::CObjectCollectionSubscriberControllerComp
{
public:
	typedef imtgql::CObjectCollectionSubscriberControllerComp BaseClass;

	I_BEGIN_COMPONENT(CServiceStatusCollectionSubscriberControllerComp);
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
	I_END_COMPONENT;

protected:
	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;

private:
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);

};


} // namespace agentinogql


