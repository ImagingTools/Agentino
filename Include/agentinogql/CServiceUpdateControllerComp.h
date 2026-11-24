// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>
#include <QtCore/QStringList>

// ImtCore includes
#include <imtbase/IObjectCollection.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/IServiceVersionFolders.h>
#include <agentinogql/IEnrollmentController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentinogql
{


/**
	GraphQL surface of the "Update" page: the service types (software products) installed on the
	agents, with the number of agents using each of them, and the operating system families of the
	agents (Windows / Linux / macOS / Other) to filter them by. It also keeps the folders the versions
	of a service type for an operating system family are chosen in ("Set service folder" of the
	versions page, ServiceVersionFolders).

	The schema lives in Sdl/agentino/1.0/Updates.sdl.
*/
class CServiceUpdateControllerComp: public sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase
{
public:
	typedef sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceUpdateControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections", true, "ServiceManager");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Enrollment records holding the operating systems reported by the agents", false, "EnrollmentStore");
		I_ASSIGN(m_serviceVersionFoldersCompPtr, "ServiceVersionFolders", "Folders the versions of the services are chosen in (GetServiceVersionFolder / SetServiceVersionFolder)", false, "ServiceVersionFolders");
	I_END_COMPONENT;

	/**
		Get the family of the operating system (as reported by the agent): "Windows", "Linux", "macOS",
		or "Other" for an unrecognized one. Empty if the operating system is not known.
		The versions of the services (CServiceVersionCollectionControllerComp) are grouped by it as well.
	*/
	static QString GetOperatingSystemFamily(const QString& operatingSystem);

protected:
	// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)
	virtual sdl::V1_0::agentino::CServiceUpdateListPayload OnServiceUpdatesList(
				const sdl::V1_0::agentino::CServiceUpdatesListGqlRequest& serviceUpdatesListRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CAgentOperatingSystemList OnAgentOperatingSystemsList(
				const sdl::V1_0::agentino::CAgentOperatingSystemsListGqlRequest& agentOperatingSystemsListRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CServiceVersionFolder OnGetServiceVersionFolder(
				const sdl::V1_0::agentino::CGetServiceVersionFolderGqlRequest& getServiceVersionFolderRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CServiceVersionFolder OnSetServiceVersionFolder(
				const sdl::V1_0::agentino::CSetServiceVersionFolderGqlRequest& setServiceVersionFolderRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		Get the family of the operating system reported by the agent. Empty if the agent did not report it.
	*/
	QString GetAgentOperatingSystemFamily(const QByteArray& agentId) const;

	/**
		Get the operating system families the agents are installed on (Windows, Linux, macOS, Other - in this order).
		\param hasUnknown	set if some agents did not report their operating system.
	*/
	QStringList GetOperatingSystemFamilies(bool& hasUnknown) const;

	/**
		Get the number of agents using each service, by service type ID.
		An agent with several services of the same type is counted once.
		A service without a type is counted under its own name.
		\param operatingSystemFamilyPtr	if set, only the agents of this operating system family are counted
										(empty: the agents that did not report their operating system).
	*/
	QMap<QString, int> GetServiceAgentCounts(const QString* operatingSystemFamilyPtr = nullptr) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);
	I_REF(agentinodata::IServiceVersionFolders, m_serviceVersionFoldersCompPtr);
};


} // namespace agentinogql
