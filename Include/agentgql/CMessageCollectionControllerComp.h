// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentgql/CServiceLog.h>

// Generated includes
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/ServiceLog.h>
#include <imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


namespace agentgql
{


class CMessageCollectionControllerComp:
									public sdl::agentino::ServiceLog::CServiceLogCollectionControllerCompBase,
									public CServiceLog
{
public:
	typedef CServiceLogCollectionControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CMessageCollectionControllerComp);
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::agentino::ServiceLog::CServiceLogCollectionControllerCompBase)
	virtual bool CreateRepresentationFromObject(
				const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
				const sdl::agentino::ServiceLog::CGetServiceLogGqlRequest& getServiceLogRequest,
				sdl::imtbase::ImtCollection::CMessageItem::V1_0& representationObject,
				QString& errorMessage) const override;
	virtual imtbase::CTreeItemModel* ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

	virtual istd::TUniqueInterfacePtr<imtbase::IObjectCollection> GetMessageCollection(const QByteArray& serviceId, QString& errorMessage) const;
};


} // namespace agentgql


