// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/IAsyncGqlClient.h>
#include <imtcom/IServerConnectionInterface.h>
#include <imtrest/IHttpFileResolver.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/IServiceVersionFolders.h>
#include <agentinodata/IServiceVersionInfo.h>
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

	"Install" of the versions page (InstallServiceVersion): the server sends the URLs of the archives
	of the version (the *.zip files of its folder) to the chosen agents - an UpdateServiceVersion request
	(Services.sdl) for every service of the type of the version on each agent. The URL of an archive is
	<HTTP address of the server>/<FileCommandId>/<version ID>/<file name>. The agent downloads the
	archives over HTTP into the folder of the executable of the service, then stops the service, unpacks
	the archives there, starts it and removes the archives (agentgql::CServiceVersionInstallerComp).

	The archives are sent by a file servlet (imtrest::CHttpFileServletComp with the command
	"<FileCommandId>/*") that asks this component for the file (imtrest::IHttpFileResolver): only the
	archives of the versions added to the server are given, to the approved agents only (header
	'clientid' of the request).

	The schema lives in Sdl/agentino/1.0/Updates.sdl.
*/
class CServiceUpdateControllerComp:
			public QObject,
			public sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase,
			virtual public imtrest::IHttpFileResolver
{
	Q_OBJECT
public:
	typedef sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceUpdateControllerComp);
		I_REGISTER_INTERFACE(imtrest::IHttpFileResolver);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections", true, "ServiceManager");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Enrollment records holding the operating systems reported by the agents", false, "EnrollmentStore");
		I_ASSIGN(m_serviceVersionFoldersCompPtr, "ServiceVersionFolders", "Folders the versions of the services are chosen in (GetServiceVersionFolder / SetServiceVersionFolder)", false, "ServiceVersionFolders");
		I_ASSIGN(m_serviceVersionCollectionCompPtr, "ServiceVersionCollection", "Versions of the services uploaded to the server (InstallServiceVersion, the archives of the versions sent over HTTP)", false, "ServiceVersionCollection");
		I_ASSIGN(m_asyncApiClientCompPtr, "AsyncApiClient", "Asynchronous API client the update requests are sent to the agents with (header 'clientid')", false, "AsyncApiClient");
		I_ASSIGN(m_serverConnectionCompPtr, "ServerConnectionInterface", "Addresses of the server: the URLs of the archives of the versions are made of its HTTP address (without it the agents get the path of a URL only)", false, "ServerConnectionInterface");
		I_ASSIGN(m_fileCommandIdAttrPtr, "FileCommandId", "HTTP path the archives of the versions are sent at: the command of the file servlet without '/*'", true, "Agentino/ServiceVersions");
	I_END_COMPONENT;

	/**
		Get the family of the operating system (as reported by the agent): "Windows", "Linux", "macOS",
		or "Other" for an unrecognized one. Empty if the operating system is not known.
		The versions of the services (CServiceVersionCollectionControllerComp) are grouped by it as well.
	*/
	static QString GetOperatingSystemFamily(const QString& operatingSystem);

	// reimplemented (imtrest::IHttpFileResolver)
	virtual imtrest::IProtocolEngine::StatusCode ResolveFile(const imtrest::CHttpRequest& request, FileDescription& file) const override;

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
	virtual sdl::V1_0::agentino::CInstallServiceVersionPayload OnInstallServiceVersion(
				const sdl::V1_0::agentino::CInstallServiceVersionGqlRequest& installServiceVersionRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		A service of an agent: ID and name.
	*/
	struct AgentService
	{
		QByteArray id;
		QString name;
	};

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

	/**
		Get the services of the agent having the service type (a service without a type is matched by its name,
		as on the "Update" page), as the server mirrors them.
	*/
	QList<AgentService> GetAgentServicesOfType(const QByteArray& agentId, const QString& serviceTypeId) const;

	/**
		Send UpdateServiceVersion (Services.sdl) for the service to the agent. The request is not waited for:
		the agent installs the version on its own; a refusal of the agent is reported to the log.
		\param archiveUrls	URLs of the archives of the version the agent downloads.
	*/
	bool SendUpdateServiceVersionRequest(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				const QString& version,
				const QStringList& archiveUrls,
				QString& errorMessage) const;

	/**
		Get the URLs of the archives (*.zip) of the folder of the version (see the class description).
		\param versionId	ID of the version in the version collection.
		\param folderPath	clean path of the folder of the version.
	*/
	QStringList GetArchiveUrls(const QByteArray& versionId, const QString& folderPath) const;

	/**
		Get the HTTP path the archives of the versions are sent at, without the slashes at the ends.
	*/
	QString GetFileCommandId() const;

	/**
		Whether the name is the name of an archive (*.zip) without a path.
	*/
	static bool IsArchiveFileName(const QString& fileName);

	/**
		Path with forward slashes, without "." / ".." parts and a trailing slash.
	*/
	static QString GetCleanPath(const QString& path);

	/**
		Whether the paths are the same: both are clean paths; on Windows the case is ignored.
	*/
	static bool IsSamePath(const QString& path1, const QString& path2);

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);
	I_REF(agentinodata::IServiceVersionFolders, m_serviceVersionFoldersCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceVersionCollectionCompPtr);
	I_REF(imtclientgql::IAsyncGqlClient, m_asyncApiClientCompPtr);
	I_REF(imtcom::IServerConnectionInterface, m_serverConnectionCompPtr);
	I_ATTR(QByteArray, m_fileCommandIdAttrPtr);
};


} // namespace agentinogql
