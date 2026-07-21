// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/FileSystem_fwd.h>


namespace agentinogql
{


/**
	Forwards GetFileSystemEntries to the agent identified by the request
	header \c clientid (same routing channel as CServiceControllerProxyComp).
 */
class CFileSystemControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
						sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CFileSystemControllerProxyComp);
	I_END_COMPONENT;

protected:
	// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)
	virtual sdl::V1_0::imtbase::CGetFileSystemEntriesPayload OnGetFileSystemEntries(
				const sdl::V1_0::imtbase::CGetFileSystemEntriesGqlRequest& getFileSystemEntriesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
};


} // namespace agentinogql
