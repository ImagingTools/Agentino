// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Standard includes
#include <memory>

// Qt includes
#include <QtCore/QHash>

// ACF includes
#include <icomp/CComponentBase.h>
#include <ilog/TLoggerCompWrap.h>

// ImtCore includes
#include <imtbase/TPluginManager.h>
#include <imtbase/PluginInterface.h>
#include <imtservice/IConnectionCollectionPlugin.h>

// Agentino includes
#include <agentinodata/IServiceTypeCatalog.h>


IMT_DECLARE_PLUGIN_INTERFACE(ServiceSettingsCatalog, imtservice::IConnectionCollectionPlugin);


namespace agentinodata
{


/**
	Caches one PluginManager per service type (not per start).
*/
class CServiceTypeCatalogComp:
			public ilog::CLoggerComponentBase,
			virtual public IServiceTypeCatalog
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceTypeCatalogComp);
		I_REGISTER_INTERFACE(IServiceTypeCatalog);
	I_END_COMPONENT;

	// reimplemented (IServiceTypeCatalog)
	virtual bool EnsureTypeLoaded(
				const QByteArray& typeId,
				const QString& pluginDirectory,
				const QString& executableBaseName,
				QString& errorMessage) override;
	virtual PluginCapability Capability(const QByteArray& typeId) const override;
	virtual QStringList LoadedTypeIds() const override;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

private:
	class PluginManager: public imtbase::TPluginManager<
				imtservice::IConnectionCollectionPlugin,
				IMT_CREATE_PLUGIN_FUNCTION(ServiceSettingsCatalog),
				IMT_DESTROY_PLUGIN_FUNCTION(ServiceSettingsCatalog)>
	{
	public:
		PluginManager(
			const QByteArray& createMethodName,
			const QByteArray& destroyMethodName,
			imtbase::IPluginStatusMonitor* pluginStatusMonitorPtr)
		{
			m_createMethodName = createMethodName;
			m_destroyMethodName = destroyMethodName;
			m_pluginStatusMonitorPtr = pluginStatusMonitorPtr;
		}
	};

	struct Entry
	{
		PluginCapability capability;
		// shared_ptr, not TDelPtr: this struct is stored BY VALUE in a QHash, which copies
		// it on insert/lookup. TDelPtr's "copy" constructor is a no-op stub (STL-container
		// compatibility only) that asserts the source is already NULL — using it here would
		// assert/crash on every insert of a loaded entry.
		std::shared_ptr<PluginManager> manager;
	};

	QHash<QByteArray, Entry> m_types;
};


} // namespace agentinodata
