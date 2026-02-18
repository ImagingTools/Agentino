// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
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
	QStringList GetConnectionInfoAboutDependOnService(const QByteArray& connectionId) const;
	QStringList GetConnectionInfoAboutServiceDepends(const QByteArray& connectionId) const;
	istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> GetConnectionInfo(const QByteArray& connectionId) const;

	virtual sdl::imtbase::ImtCollection::CGetElementMetaInfoPayload OnGetElementMetaInfo(
				const sdl::imtbase::ImtCollection::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

	// reimplemented (imtgql::CObjectCollectionControllerCompBase)
	virtual bool SetupGqlItem(
				const imtgql::CGqlRequest& gqlRequest,
				imtbase::CTreeItemModel& model,
				int itemIndex,
				const QByteArray& collectionId,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	QStringList GetDependantStatusInfo(const QByteArray& serviceId) const;

protected:
	I_REF(imtbase::IObjectCollection, m_serviceStatusCollectionCompPtr);
	I_REF(agentinodata::IServiceCompositeInfo, m_serviceCompositeInfoCompPtr);
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentinogql


