// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QStringList>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtgql/IGqlContext.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentinogql
{


/**
	Collection of the versions of a service type uploaded to the server - the table of the versions
	page of a service, opened by a double click on the "Update" page.

	The page passes the service type in the "serviceTypeId" request header; the list is restricted to
	it by the HeaderFilterHeaderIds/HeaderFilterFieldIds attributes. The name of a collection element
	is its version. The number of installations and the meta info (the agents having the version
	installed) come from the services the agents run.

	The schema lives in Sdl/agentino/1.0/Updates.sdl.
*/
class CServiceVersionCollectionControllerComp: public sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase
{
public:
	typedef sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceVersionCollectionControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections", true, "ServiceManager");
	I_END_COMPONENT;

	// reimplemented (imtservergql::CObjectCollectionControllerCompBase)
	virtual sdl::V1_0::imtbase::CGetElementMetaInfoPayload OnGetElementMetaInfo(
				const sdl::V1_0::imtbase::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

protected:
	// reimplemented (sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::V1_0::agentino::CServiceVersionsListGqlRequest& serviceVersionsListRequest,
				sdl::V1_0::agentino::CServiceVersionItem& representationObject,
				QString& errorMessage) const override;

private:
	/**
		Get the service type of the request ("serviceTypeId" header).
	*/
	static QByteArray GetServiceTypeId(const imtgql::IGqlContext* gqlContextPtr);

	/**
		Get the names of the agents having the version of the service type installed.
		A service without a type is matched by its name (as on the "Update" page).
	*/
	QStringList GetVersionAgentNames(const QByteArray& serviceTypeId, const QString& version) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
};


} // namespace agentinogql
