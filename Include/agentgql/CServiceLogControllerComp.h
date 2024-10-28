#pragma once


// ACF includes
#include <istd/TDelPtr.h>

// ImtCore includes
#include <agentgql/CServiceLog.h>
#include <imtservergql/CGqlRequestHandlerCompBase.h>


namespace agentgql
{


class CServiceLogControllerComp:
			public imtservergql::CGqlRequestHandlerCompBase,
			public CServiceLog
{
public:
	typedef imtservergql::CGqlRequestHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceLogControllerComp);
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Service collection", false, "ServiceCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtservergql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
};


} // namespace agentgql


