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

	virtual istd::TUniqueInterfacePtr<imtbase::IObjectCollection> GetMessageCollection(const QByteArray& serviceId, QString& errorMessage) const;
};


} // namespace agentgql


