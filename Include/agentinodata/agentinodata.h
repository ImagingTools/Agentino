// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QProcess>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtcom/CServerConnectionInterfaceParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>
#include <imtservice/IConnectionCollection.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes_fwd.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services_fwd.h>


namespace agentinodata
{


struct ProcessStateEnum
{
	QByteArray id;
	QString name;
};

ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState);


bool GetServiceFromRepresentation(agentinodata::CServiceInfo& serviceInfo, const sdl::V1_0::agentino::CServiceData& serviceDataRepresentation, QString& errorMessage);
bool GetRepresentationFromService(sdl::V1_0::agentino::CServiceData& serviceDataRepresentation, const agentinodata::CServiceInfo& serviceInfo, const iprm::IParamsSet* paramsPtr = nullptr);

bool GetUrlConnectionFromRepresentation(imtservice::CUrlConnectionParam& connectionInfo, const sdl::V1_0::agentino::CInputConnection& connectionRepresentation);
bool GetRepresentationFromUrlConnection(sdl::V1_0::agentino::CInputConnection& connectionRepresentation, imtservice::CUrlConnectionParam& connectionInfo, const iprm::IParamsSet* paramsPtr = nullptr);

bool GetUrlConnectionLinkFromRepresentation(imtservice::CUrlConnectionLinkParam& connectionInfo, const sdl::V1_0::agentino::COutputConnection& connectionRepresentation);
bool GetRepresentationFromUrlConnectionLink(
	sdl::V1_0::agentino::COutputConnection& connectionRepresentation,
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const iprm::IParamsSet* paramsPtr = nullptr);

bool GetServerConnectionParamFromRepresentation(
			imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			const sdl::V1_0::imtbase::CServerConnectionParam& serverConnectionRepresentation);
bool GetRepresentationFromServerConnectionParam(
			const imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			sdl::V1_0::imtbase::CServerConnectionParam& serverConnectionRepresentation);

bool GetRepresentationFromConnectionCollection(
			imtservice::IConnectionCollection& connectionCollection,
			sdl::V1_0::agentino::CPluginInfo& connectionCollectionRepresentation);

/**
	Collect Available Connections for an output slot (\a connectionUsageId) from services
	in \a serviceCollection (matching input connection ids + their extern endpoints).
	Used on agent (local services only) and server (each agent mirror).
*/
void AppendAvailableConnectionsFromServiceCollection(
			imtbase::IObjectCollection& serviceCollection,
			const QByteArray& connectionUsageId,
			QList<sdl::V1_0::agentino::CDependantConnectionInfo>& outList);

/**
	Fill outputConnections[*].dependantConnectionList for the service editor.
*/
void FillAvailableConnectionsForServiceData(
			sdl::V1_0::agentino::CServiceData& serviceData,
			imtbase::IObjectCollection& serviceCollection);

void FillAvailableConnectionsForPluginInfo(
			sdl::V1_0::agentino::CPluginInfo& pluginInfo,
			imtbase::IObjectCollection& serviceCollection);


} // namespace agentinodata


