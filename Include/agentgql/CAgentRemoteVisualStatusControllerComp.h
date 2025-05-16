#pragma once


// Agentino includes
#include <agentgql/CAgentGqlRemoteRepresentationControllerComp.h>


namespace agentgql
{


class CAgentRemoteVisualStatusControllerComp: public agentgql::CAgentGqlRemoteRepresentationControllerComp
{
public:
	typedef agentgql::CAgentGqlRemoteRepresentationControllerComp BaseClass;

	I_BEGIN_COMPONENT(CAgentRemoteVisualStatusControllerComp)
		I_ASSIGN_MULTI_0(m_typeIdsAttrPtr, "TypeIds", "Remote object type-IDs", false);
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
	
protected:
	I_MULTIATTR(QByteArray, m_typeIdsAttrPtr);
};


} // namespace agentgql


