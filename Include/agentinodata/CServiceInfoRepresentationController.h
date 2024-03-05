#pragma once


// ImtCore includes
#include <imtbase/IRepresentationController.h>
#include <imtservice/CUrlConnectionParamRepresentationController.h>
#include <imtservice/CUrlConnectionLinkParamRepresentationController.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>


namespace agentinodata
{


class CServiceInfoRepresentationController: public imtbase::IRepresentationController
{
public:
	// reimplemented (imtbase::IRepresentationController)
	virtual QByteArray GetModelId() const override;
	virtual bool IsModelSupported(const istd::IChangeable& dataModel) const override;
	virtual bool GetRepresentationFromDataModel(
				const istd::IChangeable& dataModel,
				imtbase::CTreeItemModel& representation,
				const iprm::IParamsSet* paramsPtr = nullptr) const override;
	virtual bool GetDataModelFromRepresentation(
				const imtbase::CTreeItemModel& representation,
				istd::IChangeable& dataModel) const override;

private:
	imtservice::CUrlConnectionParamRepresentationController m_urlConnectionParamRepresentationController;
	imtservice::CUrlConnectionLinkParamRepresentationController m_urlConnectionLinkParamRepresentationController;
};


} // namespace agentinodata



