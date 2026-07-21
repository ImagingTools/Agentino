// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinodata/CServiceTypeCatalogComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

// ACF includes
#include <istd/TInterfacePtr.h>


namespace agentinodata
{


// public methods

bool CServiceTypeCatalogComp::EnsureTypeLoaded(
			const QByteArray& typeId,
			const QString& pluginDirectory,
			const QString& executableBaseName,
			QString& errorMessage)
{
	const QByteArray key = typeId.isEmpty() ? executableBaseName.toUtf8() : typeId;
	if (key.isEmpty()) {
		errorMessage = QStringLiteral("Empty type id");
		return false;
	}

	if (m_types.contains(key) && m_types[key].capability.loaded) {
		return true;
	}

	Entry entry;
	entry.capability.typeId = key;
	entry.capability.pluginName = executableBaseName + QStringLiteral("Settings");

	if (pluginDirectory.isEmpty() || !QDir(pluginDirectory).exists()) {
		entry.capability.loaded = false;
		entry.capability.lastError = QStringLiteral("Plugin directory missing: %1").arg(pluginDirectory);
		m_types.insert(key, entry);
		errorMessage = entry.capability.lastError;
		return false;
	}

	entry.manager = std::shared_ptr<PluginManager>(new PluginManager(
				IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettingsCatalog),
				IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettingsCatalog),
				nullptr));

	if (!entry.manager->LoadPluginDirectory(pluginDirectory, "plugin", "ServiceSettings")) {
		entry.capability.loaded = false;
		entry.capability.lastError = QStringLiteral("Failed to load plugins from %1").arg(pluginDirectory);
		m_types.insert(key, entry);
		errorMessage = entry.capability.lastError;
		return false;
	}

	// Match plugin by name == executableBaseName + "Settings"
	const QString expectedName = entry.capability.pluginName;
	bool matched = false;
	for (int i = 0; i < entry.manager->m_plugins.count(); ++i) {
		imtservice::IConnectionCollectionPlugin* plugin = entry.manager->m_plugins[i].pluginPtr;
		if (plugin == nullptr || plugin->GetPluginName() != expectedName) {
			continue;
		}
		const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* factory =
					plugin->GetConnectionCollectionFactory();
		if (factory != nullptr) {
			istd::TUniqueInterfacePtr<imtservice::IConnectionCollection> collection =
						factory->CreateInstance();
			if (collection.IsValid()) {
				entry.capability.version = collection->GetServiceVersion();
			}
		}
		matched = true;
		break;
	}

	if (!matched) {
		// Keep managers loaded; capability still useful for discovery failures
		entry.capability.lastError = QStringLiteral("No plugin named %1").arg(expectedName);
		entry.capability.loaded = false;
		m_types.insert(key, entry);
		errorMessage = entry.capability.lastError;
		return false;
	}

	entry.capability.loaded = true;
	m_types.insert(key, entry);
	return true;
}


PluginCapability CServiceTypeCatalogComp::Capability(const QByteArray& typeId) const
{
	return m_types.value(typeId).capability;
}


QStringList CServiceTypeCatalogComp::LoadedTypeIds() const
{
	QStringList ids;
	for (auto it = m_types.constBegin(); it != m_types.constEnd(); ++it) {
		if (it.value().capability.loaded) {
			ids.append(QString::fromUtf8(it.key()));
		}
	}
	return ids;
}


// protected methods

void CServiceTypeCatalogComp::OnComponentDestroyed()
{
	m_types.clear();
	BaseClass::OnComponentDestroyed();
}


} // namespace agentinodata
