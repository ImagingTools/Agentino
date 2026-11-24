// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtgql/IGqlContext.h>

// Agentino includes
#include <agentinodata/IServiceManager.h>
#include <agentinodata/IServiceVersionFolders.h>
#include <agentinogql/IEnrollmentController.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates_fwd.h>


namespace agentinogql
{


/**
	Collection of the versions of a service type uploaded to the server - the table of the versions
	page of a service, opened by a double click on the "Update" page.

	The page passes the service type in the "serviceTypeId" request header; the list is restricted to
	the versions of it (agentinodata::IServiceVersionInfo::GetServiceTypeId). The collection is kept in
	memory, which applies no ComplexFilter, so the list request filters, sorts and pages the versions
	itself (GetObjectListFromRequest). The name of a collection element is its version. The number of
	installations and the meta info (the agents having the version installed) come from the services
	the agents run.

	Versions differ by the operating system family (Windows / Linux / macOS / Other, as on the "Update"
	page): the "operatingSystem" request header restricts the list to one family (absent or empty: all
	of them), and a version counts as installed on the agents of its own family only.

	A version is added by AddServiceVersion: a folder on the server, its name is the version; the
	operating system family, the path of the folder and the upload date are stored with it. The folder
	must lie inside the folder of the versions of its service type and operating system family
	(ServiceVersionFolders, "Set service folder").

	The schema lives in Sdl/agentino/1.0/Updates.sdl.
*/
class CServiceVersionCollectionControllerComp: public sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase
{
public:
	typedef sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CServiceVersionCollectionControllerComp);
		I_ASSIGN(m_agentCollectionCompPtr, "AgentCollection", "Agent collection", true, "AgentCollection");
		I_ASSIGN(m_serviceManagerCompPtr, "ServiceManager", "Per-agent service collections", true, "ServiceManager");
		I_ASSIGN(m_enrollmentControllerCompPtr, "EnrollmentController", "Enrollment records holding the operating systems reported by the agents", false, "EnrollmentStore");
		I_ASSIGN(m_serviceVersionFoldersCompPtr, "ServiceVersionFolders", "Folders the versions are chosen in: a version is added from inside the folder of its service type and operating system", false, "ServiceVersionFolders");
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
	virtual istd::IChangeableUniquePtr CreateObjectFromRepresentation(
				const sdl::V1_0::agentino::CServiceVersionData& serviceVersionDataRepresentation,
				QByteArray& newObjectId,
				QString& errorMessage) const override;

	// reimplemented (imtservergql::CObjectCollectionControllerCompBase)
	virtual QJsonObject GetObjectListFromRequest(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

private:
	/**
		Iterator over a given list of the elements of a collection.
	*/
	class CElementListIterator: virtual public imtbase::IObjectCollectionIterator
	{
	public:
		CElementListIterator(const imtbase::IObjectCollection& collection, const imtbase::ICollectionInfo::Ids& ids, int elementsCount);

		// reimplemented (imtbase::IObjectCollectionIterator)
		virtual bool Next() const override;
		virtual bool Previous() const override;
		virtual QByteArray GetObjectId() const override;
		virtual QByteArray GetObjectTypeId() const override;
		virtual bool GetObjectData(imtbase::IObjectCollection::DataPtr& dataPtr) const override;
		virtual idoc::MetaInfoPtr GetDataMetaInfo() const override;
		virtual int GetElementsCount() const override;
		virtual idoc::MetaInfoPtr GetCollectionMetaInfo() const override;
		virtual QVariant GetElementInfo(int infoType) const override;
		virtual QVariant GetElementInfo(QByteArray infoId) const override;

	private:
		bool IsValid() const;

	private:
		const imtbase::IObjectCollection& m_collection;
		imtbase::ICollectionInfo::Ids m_ids;
		int m_elementsCount;
		mutable int m_index;
	};

	struct VersionElement
	{
		QByteArray id;
		QString operatingSystem;
		QString version;
		QDateTime uploadDate;
	};

	/**
		Get the versions of the service type stored in the collection.
		\param operatingSystemPtr	if set, only the versions of this operating system family.
	*/
	QList<VersionElement> GetVersionElements(const QByteArray& serviceTypeId, const QString* operatingSystemPtr = nullptr) const;

	/**
		Compare two versions part by part, numerically where both parts are numbers ("1.10" > "1.9").
		\return negative, zero or positive value as for QString::compare.
	*/
	static int CompareVersions(const QString& version1, const QString& version2);

	/**
		Whether the path lies inside the folder (strictly: the folder itself is not inside it).
		Both are clean paths with forward slashes; on Windows the case is ignored.
	*/
	static bool IsPathInsideFolder(const QString& path, const QString& folder);

	/**
		Get the service type of the request ("serviceTypeId" header).
	*/
	static QByteArray GetServiceTypeId(const imtgql::IGqlContext* gqlContextPtr);

	/**
		Get the operating system family of the request ("operatingSystem" header). Empty: all of them.
	*/
	static QString GetOperatingSystem(const imtgql::IGqlContext* gqlContextPtr);

	/**
		Get the operating system family of the agent (see CServiceUpdateControllerComp::GetOperatingSystemFamily).
		Empty if the agent did not report its operating system.
	*/
	QString GetAgentOperatingSystem(const QByteArray& agentId) const;

	/**
		Get the names of the agents having the version of the service type installed.
		A service without a type is matched by its name (as on the "Update" page).
		\param operatingSystem	if not empty, only the agents of this operating system family.
	*/
	QStringList GetVersionAgentNames(const QByteArray& serviceTypeId, const QString& version, const QString& operatingSystem) const;

protected:
	I_REF(imtbase::IObjectCollection, m_agentCollectionCompPtr);
	I_REF(agentinodata::IServiceManager, m_serviceManagerCompPtr);
	I_REF(IEnrollmentController, m_enrollmentControllerCompPtr);
	I_REF(agentinodata::IServiceVersionFolders, m_serviceVersionFoldersCompPtr);
};


} // namespace agentinogql
