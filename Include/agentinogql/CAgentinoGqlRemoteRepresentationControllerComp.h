#pragma once

// ImtCore includes
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>


namespace agentinogql
{


class CAgentinoGqlRemoteRepresentationControllerComp: public imtclientgql::CGqlRemoteRepresentationControllerCompBase
{
public:
	typedef imtclientgql::CGqlRemoteRepresentationControllerCompBase BaseClass;

	I_BEGIN_COMPONENT(CAgentinoGqlRemoteRepresentationControllerComp)
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
};


} // namespace agentinogql


