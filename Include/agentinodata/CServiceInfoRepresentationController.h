#pragma once


// ImtCore includes
#include <imtrest/IRepresentationController.h>
#include <imtservice/CUrlConnectionParamRepresentationController.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CUrlConnectionLinkParamRepresentationController.h>


namespace agentinodata
{


class CServiceInfoRepresentationController: public imtrest::IRepresentationController
{
public:
	// reimplemented (imtrest::IRepresentationController)
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
	agentinodata::CUrlConnectionLinkParamRepresentationController m_urlConnectionLinkParamRepresentationController;
};


} // namespace agentinodata



