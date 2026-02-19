// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QProcess>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtcom/CServerConnectionInterfaceParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>
#include <imtservice/IConnectionCollection.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


namespace agentinodata
{


struct ProcessStateEnum
{
	QByteArray id;
	QString name;
};

ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState);


bool GetServiceFromRepresentation(agentinodata::CServiceInfo& serviceInfo, const sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation, QString& errorMessage);
bool GetRepresentationFromService(sdl::agentino::Services::CServiceData::V1_0& serviceDataRepresentation, const agentinodata::CServiceInfo& serviceInfo, const iprm::IParamsSet* paramsPtr = nullptr);

bool GetUrlConnectionFromRepresentation(imtservice::CUrlConnectionParam& connectionInfo, const sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation);
bool GetRepresentationFromUrlConnection(sdl::agentino::Services::CInputConnection::V1_0& connectionRepresentation, imtservice::CUrlConnectionParam& connectionInfo, const iprm::IParamsSet* paramsPtr = nullptr);

bool GetUrlConnectionLinkFromRepresentation(imtservice::CUrlConnectionLinkParam& connectionInfo, const sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation);
bool GetRepresentationFromUrlConnectionLink(
	sdl::agentino::Services::COutputConnection::V1_0& connectionRepresentation,
	imtservice::CUrlConnectionLinkParam& connectionInfo,
	const iprm::IParamsSet* paramsPtr = nullptr);

bool GetServerConnectionParamFromRepresentation(
			imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			const sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& serverConnectionRepresentation);
bool GetRepresentationFromServerConnectionParam(
			const imtcom::CServerConnectionInterfaceParam& serverConnectionParam,
			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& serverConnectionRepresentation);

bool GetRepresentationFromConnectionCollection(
			imtservice::IConnectionCollection& connectionCollection,
			sdl::agentino::Services::CPluginInfo::V1_0& connectionCollectionRepresentation);


} // namespace agentinodata


