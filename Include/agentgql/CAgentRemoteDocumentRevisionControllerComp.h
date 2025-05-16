#pragma once


// Agentino includes
#include <agentgql/CAgentGqlRemoteRepresentationControllerComp.h>


namespace agentgql
{


class CAgentRemoteDocumentRevisionControllerComp: public agentgql::CAgentGqlRemoteRepresentationControllerComp
{
public:
	typedef agentgql::CAgentGqlRemoteRepresentationControllerComp BaseClass;

	I_BEGIN_COMPONENT(CAgentRemoteDocumentRevisionControllerComp)
		I_ASSIGN_MULTI_0(m_collectionIdsAttrPtr, "CollectionIds", "Collection ID-s", false);
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
	
protected:
	I_MULTIATTR(QByteArray, m_collectionIdsAttrPtr);
};


} // namespace agentgql


