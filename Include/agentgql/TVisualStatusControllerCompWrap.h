// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


namespace agentgql
{


template <class RemoteControllerComp>
class TVisualStatusControllerCompWrap: public RemoteControllerComp
{
public:
	typedef RemoteControllerComp BaseClass;

	I_BEGIN_COMPONENT(TVisualStatusControllerCompWrap)
		I_ASSIGN_MULTI_0(m_typeIdsAttrPtr, "TypeIds", "Remote object type-IDs", false);
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;

protected:
	I_MULTIATTR(QByteArray, m_typeIdsAttrPtr);
};


// public methods

template<class RemoteControllerComp>
bool TVisualStatusControllerCompWrap<RemoteControllerComp>::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool isSupported = BaseClass::IsRequestSupported(gqlRequest);
	if (!isSupported){
		return false;
	}

	sdl::V1_0::imtbase::CGetObjectVisualStatusGqlRequest getVisualStatusRequest(gqlRequest, false);
	if (getVisualStatusRequest.IsValid()){
		sdl::V1_0::imtbase::GetObjectVisualStatusRequestArguments arguments = getVisualStatusRequest.GetRequestedArguments();
		if (!arguments.input.has_value()){
			return false;
		}

		QByteArray typeId;
		if (arguments.input->typeId){
			typeId = *getVisualStatusRequest.GetRequestedArguments().input->typeId;
		}

		return m_typeIdsAttrPtr.FindValue(typeId) >= 0;
	}

	return true;
}


} // namespace agentgql


