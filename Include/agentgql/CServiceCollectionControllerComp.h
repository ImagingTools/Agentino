#pragma once

// ImtCore includes
#include <imtgql/CObjectCollectionControllerCompBase.h>

// ServiceManager includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/IServiceController.h>

#undef GetObject


namespace agentgql
{


class CServiceCollectionControllerComp: public imtgql::CObjectCollectionControllerCompBase
{
public:
	typedef imtgql::CObjectCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceCollectionControllerComp);
		I_ASSIGN(m_serviceInfoFactCompPtr, "ServiceFactory", "Factory used for creation of the new service instance", true, "ServiceFactory");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Service controller used to manage services", true, "ServiceController");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
			const imtgql::CGqlRequest& gqlRequest,
			imtbase::CTreeItemModel& model,
			int itemIndex,
			const QByteArray& collectionId,
			QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual istd::IChangeable* CreateObject(const QList<imtgql::CGqlObject>& inputParams, QByteArray &objectId, QString &name, QString &description, QString& errorMessage) const override;

protected:
	I_FACT(agentinodata::IServiceInfo, m_serviceInfoFactCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
};


} // namespace agentgql


