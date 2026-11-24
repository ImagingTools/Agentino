// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtgql/IGqlRequestHandler.h>
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/FileSystem_fwd.h>


namespace agentinogql
{


/**
	Forwards GetFileSystemEntries to the agent identified by the request
	header \c clientid (same routing channel as CServiceControllerProxyComp).
	A request without the header browses the file system of the server itself
	(e.g. the folders of the service versions on the "Update" page) when the
	'LocalFileSystemController' reference is set; otherwise it is refused.
 */
class CFileSystemControllerProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<
						sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase>
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CFileSystemControllerProxyComp);
		I_ASSIGN(m_localFileSystemControllerCompPtr, "LocalFileSystemController", "Handler of the requests without the 'clientid' header: the file system of the server", false, "LocalFileSystemController");
	I_END_COMPONENT;

	// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)
	virtual QJsonObject CreateInternalResponse(const ::imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const override;

protected:
	// reimplemented (sdl::V1_0::imtbase::CFileSystemGqlHandlerCompBase)
	virtual sdl::V1_0::imtbase::CGetFileSystemEntriesPayload OnGetFileSystemEntries(
				const sdl::V1_0::imtbase::CGetFileSystemEntriesGqlRequest& getFileSystemEntriesRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	/**
		Get the agent the request is routed to (request header \c clientid).
	*/
	static QByteArray GetClientId(const ::imtgql::CGqlRequest& gqlRequest);

private:
	I_REF(imtgql::IGqlRequestHandler, m_localFileSystemControllerCompPtr);
};


} // namespace agentinogql
