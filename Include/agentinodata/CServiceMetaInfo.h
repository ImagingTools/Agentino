#pragma once


// ACF includes
#include <iser/ISerializable.h>

// ServiceManager includes
#include <agentinodata/IServiceMetaInfo.h>


namespace agentinodata
{


class CServiceMetaInfo: virtual public IServiceMetaInfo, virtual public iser::ISerializable
{
public:
	// reimplemented (IServiceMetaInfo)
	virtual QList<Dependency> GetDependensies() const override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive &archive) override;

	// reimplemented (iser::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable &object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual istd::IChangeable *CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

private:
	QList<Dependency> m_dependencyList;
};


} // namespace agentinodata




