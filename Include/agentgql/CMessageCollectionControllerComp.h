#pragma once


// ImtCore includes
#include <imtgql/CObjectCollectionControllerCompBase.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IObjectCollectionPlugin.h>
#include <imtbase/TPluginManager.h>

// Agentino includes
#include <agentgql/CServiceLog.h>
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceController.h>

#undef GetObject


namespace agentgql
{


class CMessageCollectionControllerComp:
			public imtgql::CObjectCollectionControllerCompBase,
			public CServiceLog
{
public:
	typedef imtgql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CMessageCollectionControllerComp);
		I_ASSIGN(m_serviceInfoFactCompPtr, "ServiceFactory", "Factory used for creation of the new service instance", false, "ServiceFactory");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", false, "ServiceController");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const imtbase::IObjectCollectionIterator* objectCollectionIterator,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual void SetObjectFilter(const imtgql::CGqlRequest& gqlRequest,
								 const imtbase::CTreeItemModel& objectFilterModel,
								 iprm::CParamsSet& filterParams) const override;

	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

	virtual imtbase::IObjectCollection* GetMessageCollection(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const;
protected:
	I_FACT(agentinodata::IServiceInfo, m_serviceInfoFactCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
};


} // namespace agentgql


