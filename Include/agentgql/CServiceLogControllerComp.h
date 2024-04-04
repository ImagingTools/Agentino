#pragma once


// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <agentgql/CServiceLog.h>


namespace agentgql
{


class CServiceLogControllerComp:
			public imtgql::CGqlRequestHandlerCompBase,
			public CServiceLog
{
public:
	typedef imtgql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceLogControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", false, "ServiceCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
};


} // namespace agentgql


