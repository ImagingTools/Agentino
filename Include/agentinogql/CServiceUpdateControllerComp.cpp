// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceUpdateControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>


// Qt includes
#include <QtCore/QSet>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceUpdateListPayload CServiceUpdateControllerComp::OnServiceUpdatesList(
			const sdl::V1_0::agentino::CServiceUpdatesListGqlRequest& /*serviceUpdatesListRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceUpdateListPayload payload;

	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the service list. Error: Component references 'AgentCollection' or 'ServiceManager' were not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	QList<sdl::V1_0::agentino::CServiceUpdateItem> items;

	const QMap<QString, int> serviceAgentCounts = GetServiceAgentCounts();
	for (QMap<QString, int>::const_iterator iter = serviceAgentCounts.constBegin(); iter != serviceAgentCounts.constEnd(); ++iter){
		sdl::V1_0::agentino::CServiceUpdateItem item;
		item.id = iter.key().toUtf8();
		item.name = iter.key();
		item.agentCount = iter.value();
		item.latestVersion = QString();

		items << item;
	}

	payload.items.Emplace();
	payload.items->FromList(items);

	return payload;
}


// private methods

QMap<QString, int> CServiceUpdateControllerComp::GetServiceAgentCounts() const
{
	QMap<QString, int> retVal;

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
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
