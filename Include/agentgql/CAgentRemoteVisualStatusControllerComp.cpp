#include <agentgql/CAgentRemoteVisualStatusControllerComp.h>


// ImtCore includes
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


namespace agentgql
{


// protected methods

// reimplemented (imtgql::IGqlRequestHandler)

bool CAgentRemoteVisualStatusControllerComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool isSupported = BaseClass::IsRequestSupported(gqlRequest);
	if (!isSupported){
		return false;
	}
	
	sdl::imtbase::ImtCollection::CGetObjectVisualStatusGqlRequest getVisualStatusRequest(gqlRequest, false);
	if (getVisualStatusRequest.IsValid()){
		sdl::imtbase::ImtCollection::GetObjectVisualStatusRequestArguments arguments = getVisualStatusRequest.GetRequestedArguments();
		if (!arguments.input.Version_1_0.has_value()){
			return false;
		}
		
		QByteArray typeId;
		if (arguments.input.Version_1_0->typeId){
			typeId = *getVisualStatusRequest.GetRequestedArguments().input.Version_1_0->typeId;
		}
		
		return m_typeIdsAttrPtr.FindValue(typeId) >= 0;
	}
	
	return true;
}


} // namespace agentgql


