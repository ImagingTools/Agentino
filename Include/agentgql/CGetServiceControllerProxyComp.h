#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentgql
{


class CGetServiceControllerProxyComp: public imtclientgql::CGqlRemoteRepresentationControllerCompBase
{
public:
	typedef imtclientgql::CGqlRemoteRepresentationControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CGetServiceControllerProxyComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual imtbase::CTreeItemModel* CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	virtual imtbase::CTreeItemModel* GetRepresentationModelFromServiceInfo(const imtbase::IObjectCollection& serviceCollection, const QByteArray& serviceId) const;
	virtual imtbase::CTreeItemModel* GetConnectionsModel(const QByteArray& connectionId) const;
	virtual bool GetConnectionObjectData(
				const imtbase::IObjectCollection::Id& connectionId,
				imtbase::IObjectCollection::DataPtr& connectionDataPtr,
				QString& connectionName,
				QString& connectionDescription) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
};


} // namespace agentgql


