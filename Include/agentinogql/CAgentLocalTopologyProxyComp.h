// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>

// Agentino includes
#include <agentinogql/IAgentLocalTopologyProxy.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/AgentLocalTopology_fwd.h>


namespace agentinogql
{


/**
	Server-side proxy that forwards GetLocalTopology and SaveLocalTopology
	GQL requests to individual agents via the standard ApiClient/clientid routing.

	The central CTopologyControllerComp uses this proxy to:
	 - Retrieve each agent's local service positions for the aggregated topology view.
	 - Push updated positions back to agents so they remain the source of truth.
*/
class CAgentLocalTopologyProxyComp:
			public imtclientgql::TClientRequestManagerCompWrap<sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase>,
			virtual public IAgentLocalTopologyProxy
{
public:
	typedef imtclientgql::TClientRequestManagerCompWrap<
				sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase> BaseClass;

	I_BEGIN_COMPONENT(CAgentLocalTopologyProxyComp);
		I_REGISTER_INTERFACE(agentinogql::IAgentLocalTopologyProxy)
	I_END_COMPONENT;

	// reimplemented (agentinogql::IAgentLocalTopologyProxy)
	virtual sdl::V1_0::agentino::CLocalTopology QueryLocalTopology(
				const QByteArray& agentId,
				QString& errorMessage) const override;
	virtual bool PushLocalTopology(
				const QByteArray& agentId,
				const sdl::V1_0::agentino::CLocalTopology& topology,
				QString& errorMessage) const override;

protected:
	// reimplemented (sdl::V1_0::agentino::CAgentLocalTopologyGqlHandlerCompBase)
	// These are abstract in the base but are never called on the server side;
	// request dispatching goes through CreateInternalResponse below.
	virtual sdl::V1_0::agentino::CLocalTopology OnGetLocalTopology(
				const sdl::V1_0::agentino::CGetLocalTopologyGqlRequest& getLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;
	virtual sdl::V1_0::agentino::CSaveLocalTopologyResponse OnSaveLocalTopology(
				const sdl::V1_0::agentino::CSaveLocalTopologyGqlRequest& saveLocalTopologyRequest,
				const ::imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

	// reimplemented (imtgql::CGqlRequestHandlerCompBase)
	virtual QJsonObject CreateInternalResponse(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage) const override;

private:
	template<class SdlGqlRequest, class SdlResponse>
	QJsonObject CreateProxyResponse(
				const imtgql::CGqlRequest& gqlRequest,
				QString& errorMessage,
				std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const;
};


} // namespace agentinogql
