#pragma once


// Agentino includes
#include <agentgql/CServiceCollectionControllerComp.h>
#include <agentinodata/CServiceCompositeInfoComp.h>


namespace agentinogql
{


class CServerServiceCollectionControllerComp: public agentgql::CServiceCollectionControllerComp
{
public:
	typedef agentgql::CServiceCollectionControllerComp BaseClass;

	I_BEGIN_COMPONENT(CServerServiceCollectionControllerComp);
		I_ASSIGN(m_serviceStatusCollectionCompPtr, "ServiceStatusCollection", "Service status collection", true, "ServiceStatusCollection");
		I_ASSIGN(m_serviceCompositeInfoCompPtr, "ServiceCompositeInfo", "Service composite info", true, "ServiceCompositeInfo");
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
	I_END_COMPONENT;

protected:
	QUrl GetUrlByDependantId(const QByteArray& dependantId) const;
	QStringList GetConnectionInfoAboutDependOnService(const QUrl& url, const QByteArray& connectionId) const;
	QStringList GetConnectionInfoAboutServiceDepends(const QByteArray& connectionId) const;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* GetMetaInfo(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	QString GetDependantStatusInfo(const QByteArray& serviceId) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentinogql


