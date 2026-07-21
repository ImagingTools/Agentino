// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QJsonObject>

// Agentino includes
#include <agentgql/CServiceLog.h>

// Generated includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/ServiceLog_fwd.h>
#include <imtbasesdl/SDL/1.0/CPP/ImtCollection_fwd.h>


namespace agentgql
{


class CMessageCollectionControllerComp:
									public sdl::V1_0::agentino::CServiceLogCollectionControllerCompBase,
									public CServiceLog
{
public:
	typedef CServiceLogCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CMessageCollectionControllerComp);
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::agentino::CServiceLogCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::V1_0::agentino::CGetServiceLogGqlRequest& getServiceLogRequest,
				sdl::V1_0::imtbase::CMessageItem& representationObject,
				QString& errorMessage) const override;
	virtual QJsonObject ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

	virtual istd::TSharedInterfacePtr<imtbase::IObjectCollection> GetMessageCollection(const QByteArray& serviceId, QString& errorMessage) const;

private:
	// Caches the collection opened for a given service ('m_pluginMap' only caches the loaded
	// plug-in/factory). Without this, GetMessageCollection() - called once per row from
	// CreateRepresentationFromObject() in addition to once from ListObjects() - would call
	// the plug-in factory's CreateInstance() again for every single log row, re-opening the
	// underlying storage (e.g. a SQLite connection) each time.
	typedef QMap<QByteArray, istd::TSharedInterfacePtr<imtbase::IObjectCollection>> MessageCollectionMap;
	mutable MessageCollectionMap m_messageCollectionMap;
};


} // namespace agentgql


