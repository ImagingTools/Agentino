// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceUpdateControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// Operating system families shown on the "Update" page, in the order of the panel.
static const QString s_windowsFamily = QStringLiteral("Windows");
static const QString s_linuxFamily = QStringLiteral("Linux");
static const QString s_macOsFamily = QStringLiteral("macOS");
static const QString s_otherFamily = QStringLiteral("Other");


// protected methods

// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceUpdateListPayload CServiceUpdateControllerComp::OnServiceUpdatesList(
			const sdl::V1_0::agentino::CServiceUpdatesListGqlRequest& serviceUpdatesListRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceUpdateListPayload payload;

	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the service list. Error: Component references 'AgentCollection' or 'ServiceManager' were not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::ServiceUpdatesListRequestArguments arguments = serviceUpdatesListRequest.GetRequestedArguments();

	QString operatingSystemFamily;
	const QString* operatingSystemFamilyPtr = nullptr;
	if (arguments.input.has_value() && arguments.input->operatingSystem.has_value()){
		operatingSystemFamily = *arguments.input->operatingSystem;
		operatingSystemFamilyPtr = &operatingSystemFamily;
	}

	QList<sdl::V1_0::agentino::CServiceUpdateItem> items;

	const QMap<QString, int> serviceAgentCounts = GetServiceAgentCounts(operatingSystemFamilyPtr);
	for (QMap<QString, int>::const_iterator iter = serviceAgentCounts.constBegin(); iter != serviceAgentCounts.constEnd(); ++iter){
		sdl::V1_0::agentino::CServiceUpdateItem item;
		item.id = iter.key().toUtf8();
		item.name = iter.key();
		item.agentCount = iter.value();
		item.latestVersion = QString();
		// item.autoUpdate is not provided yet: left unset, the page shows no icon.

		items << item;
	}

	payload.items.Emplace();
	payload.items->FromList(items);

	return payload;
}


sdl::V1_0::agentino::CAgentOperatingSystemList CServiceUpdateControllerComp::OnAgentOperatingSystemsList(
			const sdl::V1_0::agentino::CAgentOperatingSystemsListGqlRequest& /*agentOperatingSystemsListRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CAgentOperatingSystemList payload;

	if (!m_agentCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the operating systems of the agents. Error: Component reference 'AgentCollection' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	bool hasUnknown = false;
	const QStringList families = GetOperatingSystemFamilies(hasUnknown);

	payload.names.Emplace();
	payload.names->FromList(families);
	payload.hasUnknown = hasUnknown;

	return payload;
}


sdl::V1_0::agentino::CServiceVersionFolder CServiceUpdateControllerComp::OnGetServiceVersionFolder(
			const sdl::V1_0::agentino::CGetServiceVersionFolderGqlRequest& getServiceVersionFolderRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFolder payload;

	if (!m_serviceVersionFoldersCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the folder of the service versions. Error: Component reference 'ServiceVersionFolders' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::GetServiceVersionFolderRequestArguments arguments = getServiceVersionFolderRequest.GetRequestedArguments();

	QByteArray serviceTypeId;
	QString operatingSystem;
	if (arguments.input.has_value()){
		if (arguments.input->serviceTypeId.has_value()){
			serviceTypeId = *arguments.input->serviceTypeId;
		}
		if (arguments.input->operatingSystem.has_value()){
			operatingSystem = arguments.input->operatingSystem->trimmed();
		}
	}

	payload.serviceTypeId = serviceTypeId;
	payload.operatingSystem = operatingSystem;
	payload.path = QDir::toNativeSeparators(m_serviceVersionFoldersCompPtr->GetServiceFolder(serviceTypeId, operatingSystem));

	return payload;
}


sdl::V1_0::agentino::CServiceVersionFolder CServiceUpdateControllerComp::OnSetServiceVersionFolder(
			const sdl::V1_0::agentino::CSetServiceVersionFolderGqlRequest& setServiceVersionFolderRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFolder payload;

	if (!m_serviceVersionFoldersCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: Component reference 'ServiceVersionFolders' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::SetServiceVersionFolderRequestArguments arguments = setServiceVersionFolderRequest.GetRequestedArguments();

	QByteArray serviceTypeId;
	QString operatingSystem;
	QString path;
	if (arguments.input.has_value()){
		if (arguments.input->serviceTypeId.has_value()){
			serviceTypeId = *arguments.input->serviceTypeId;
		}
		if (arguments.input->operatingSystem.has_value()){
			operatingSystem = arguments.input->operatingSystem->trimmed();
		}
		if (arguments.input->path.has_value()){
			path = QDir::cleanPath(QDir::fromNativeSeparators(arguments.input->path->trimmed()));
		}
	}

	if (serviceTypeId.isEmpty() || operatingSystem.isEmpty()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: The service type or the operating system is not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	if (path.isEmpty() || !QFileInfo(path).isDir()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: The folder '%1' does not exist").arg(QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	// The model is stored by its auto persistence on the change.
	m_serviceVersionFoldersCompPtr->SetServiceFolder(serviceTypeId, operatingSystem, path);

	payload.serviceTypeId = serviceTypeId;
	payload.operatingSystem = operatingSystem;
	payload.path = QDir::toNativeSeparators(path);

	return payload;
}


// public static methods

QString CServiceUpdateControllerComp::GetOperatingSystemFamily(const QString& operatingSystem)
{
	if (operatingSystem.trimmed().isEmpty()){
		return QString();
	}

	if (operatingSystem.contains(QStringLiteral("windows"), Qt::CaseInsensitive)){
		return s_windowsFamily;
	}

	static const QStringList macOsKeys = {
		QStringLiteral("macos"), QStringLiteral("mac os"), QStringLiteral("os x"), QStringLiteral("osx")
	};
	for (const QString& key : macOsKeys){
		if (operatingSystem.contains(key, Qt::CaseInsensitive)){
			return s_macOsFamily;
		}
	}

	// QSysInfo::prettyProductName() of a Linux system is PRETTY_NAME of /etc/os-release,
	// which does not always mention "Linux" (e.g. "Ubuntu 24.04 LTS").
	static const QStringList linuxKeys = {
		QStringLiteral("linux"), QStringLiteral("ubuntu"), QStringLiteral("debian"), QStringLiteral("fedora"),
		QStringLiteral("centos"), QStringLiteral("red hat"), QStringLiteral("rhel"), QStringLiteral("suse"),
		QStringLiteral("mint"), QStringLiteral("manjaro"), QStringLiteral("alpine"), QStringLiteral("rocky"),
		QStringLiteral("raspbian"), QStringLiteral("gentoo"), QStringLiteral("kali"), QStringLiteral("elementary"),
		QStringLiteral("pop!_os"), QStringLiteral("zorin"), QStringLiteral("astra")
	};
	for (const QString& key : linuxKeys){
		if (operatingSystem.contains(key, Qt::CaseInsensitive)){
			return s_linuxFamily;
		}
	}

	return s_otherFamily;
}


// private methods

QString CServiceUpdateControllerComp::GetAgentOperatingSystemFamily(const QByteArray& agentId) const
{
	// The agent reports its OS with every announce; the enrollment store keeps the latest value.
	if (!m_enrollmentControllerCompPtr.IsValid()){
		return QString();
	}

	return GetOperatingSystemFamily(m_enrollmentControllerCompPtr->Get(agentId).os);
}


QStringList CServiceUpdateControllerComp::GetOperatingSystemFamilies(bool& hasUnknown) const
{
	hasUnknown = false;

	QSet<QString> families;

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		const QString family = GetAgentOperatingSystemFamily(agentId);
		if (family.isEmpty()){
			hasUnknown = true;
		}
		else{
			families.insert(family);
		}
	}

	QStringList retVal;
	for (const QString& family : {s_windowsFamily, s_linuxFamily, s_macOsFamily, s_otherFamily}){
		if (families.contains(family)){
			retVal << family;
		}
	}

	return retVal;
}


QMap<QString, int> CServiceUpdateControllerComp::GetServiceAgentCounts(const QString* operatingSystemFamilyPtr) const
{
	QMap<QString, int> retVal;

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		if (operatingSystemFamilyPtr != nullptr && GetAgentOperatingSystemFamily(agentId) != *operatingSystemFamilyPtr){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		QSet<QString> agentServiceTypes;

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			QString serviceType = serviceInfoPtr->GetServiceTypeId();
			if (serviceType.isEmpty()){
				serviceType = serviceInfoPtr->GetServiceName();
			}

			if (!serviceType.isEmpty()){
				agentServiceTypes.insert(serviceType);
			}
		}

		for (const QString& serviceType : agentServiceTypes){
			retVal[serviceType]++;
		}
	}

	return retVal;
}


} // namespace agentinogql
