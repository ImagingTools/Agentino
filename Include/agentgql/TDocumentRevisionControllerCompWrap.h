#pragma once


// ImtCore includes
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/DocumentRevision.h>


namespace agentgql
{


template <class RemoteControllerComp>
class TDocumentRevisionControllerCompWrap: public RemoteControllerComp
{
public:
	typedef RemoteControllerComp BaseClass;

	I_BEGIN_COMPONENT(TDocumentRevisionControllerCompWrap)
		I_ASSIGN_MULTI_0(m_collectionIdsAttrPtr, "CollectionIds", "Collection ID-s", false);
	I_END_COMPONENT;

protected:
	// reimplemented (imtgql::IGqlRequestHandler)
	virtual bool IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const override;
	
protected:
	I_MULTIATTR(QByteArray, m_collectionIdsAttrPtr);
};


// public methods

template<class RemoteControllerComp>
bool TDocumentRevisionControllerCompWrap<RemoteControllerComp>::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	bool isSupported = BaseClass::IsRequestSupported(gqlRequest);
	if (!isSupported){
		return false;
	}
	
	const imtgql::CGqlParamObject* inputObjectPtr = gqlRequest.GetParamObject("input");
	if (inputObjectPtr == nullptr){
		return false;
	}
	
	QByteArray collectionId = inputObjectPtr->GetParamArgumentValue(
												sdl::imtbase::DocumentRevision::CGetRevisionInfoListInput::V1_0::GetRevisionInfoListInputFields::CollectionId.toUtf8()).toByteArray();
	if (collectionId.isEmpty()){
		return false;
	}
	
	return m_collectionIdsAttrPtr.FindValue(collectionId) >= 0;
}



} // namespace agentgql


