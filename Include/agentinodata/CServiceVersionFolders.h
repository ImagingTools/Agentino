// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QMap>
#include <QtCore/QPair>

// ACF includes
#include <iser/ISerializable.h>

// Agentino includes
#include <agentinodata/IServiceVersionFolders.h>


namespace agentinodata
{


class CServiceVersionFolders:
			virtual public IServiceVersionFolders,
			virtual public iser::ISerializable
{
public:
	// reimplemented (agentinodata::IServiceVersionFolders)
	virtual QString GetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem) const override;
	virtual void SetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem, const QString& path) override;

	// reimplemented (iser::ISerializable)
	virtual bool Serialize(iser::IArchive& archive) override;

	// reimplemented (istd::IChangeable)
	virtual int GetSupportedOperations() const override;
	virtual bool CopyFrom(const IChangeable& object, CompatibilityMode mode = CM_WITHOUT_REFS) override;
	virtual bool IsEqual(const istd::IChangeable& object) const override;
	virtual istd::IChangeableUniquePtr CloneMe(CompatibilityMode mode = CM_WITHOUT_REFS) const override;
	virtual bool ResetData(CompatibilityMode mode = CM_WITHOUT_REFS) override;

protected:
	// Key: the service type ID and the operating system family.
	typedef QPair<QByteArray, QString> FolderKey;
	typedef QMap<FolderKey, QString> Folders;

	Folders m_folders;
};


} // namespace agentinodata


