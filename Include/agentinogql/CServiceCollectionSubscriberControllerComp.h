// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtservergql/CObjectCollectionChangeNotifierComp.h>


namespace agentinogql
{


class CServiceCollectionSubscriberControllerComp: public imtservergql::CObjectCollectionChangeNotifierComp
{
public:
	typedef imtservergql::CObjectCollectionChangeNotifierComp BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionSubscriberControllerComp);
	I_END_COMPONENT;

	// reimplemented (imod::CSingleModelObserverBase)
	virtual void OnUpdate(const istd::IChangeable::ChangeSet& changeSet) override;
};


} // namespace agentinodata
