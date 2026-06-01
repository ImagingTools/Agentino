// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>

// ACF includes
#include <ilog/TLoggerCompWrap.h>

// Agentino includes
#include <agentinodata/ILocalTopologyInfo.h>


namespace agentinodata
{


/**
	Agent-local topology storage component.
	Persists service position data to a JSON file in the application directory,
	making each agent the source of truth for its own service layout.
*/
class CLocalTopologyInfoComp:
			public ilog::CLoggerComponentBase,
			virtual public ILocalTopologyInfo
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CLocalTopologyInfoComp);
		I_REGISTER_INTERFACE(agentinodata::ILocalTopologyInfo)
	I_END_COMPONENT;

	// reimplemented (agentinodata::ILocalTopologyInfo)
	virtual QPoint GetServicePosition(const QByteArray& serviceId) const override;
	virtual bool SetServicePosition(const QByteArray& serviceId, const QPoint& point) override;
	virtual QByteArrayList GetServiceIds() const override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentCreated() override;

private:
	bool LoadFromFile();
	bool SaveToFile() const;
	QString GetTopologyFilePath() const;

	QMap<QByteArray, QPoint> m_positions;
};


} // namespace agentinodata
