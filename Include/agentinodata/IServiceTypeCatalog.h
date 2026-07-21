// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{


struct PluginCapability
{
	QByteArray typeId;
	QString version;
	QString pluginName;
	bool loaded = false;
	QString lastError;
};


/**
	Loads service-type plugins once per type and caches capability metadata.
*/
class IServiceTypeCatalog: virtual public istd::IChangeable
{
public:
	virtual bool EnsureTypeLoaded(
				const QByteArray& typeId,
				const QString& pluginDirectory,
				const QString& executableBaseName,
				QString& errorMessage) = 0;

	virtual PluginCapability Capability(const QByteArray& typeId) const = 0;
	virtual QStringList LoadedTypeIds() const = 0;
};


} // namespace agentinodata
