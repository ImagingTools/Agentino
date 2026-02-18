// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtserverapp/IRepresentationController.h>
#include <imtservice/CUrlConnectionLinkParam.h>


namespace agentinodata
{


class CUrlConnectionLinkParamRepresentationController: public imtserverapp::IRepresentationController
{
protected:
	virtual QUrl GetDependantConnectionUrl(imtbase::IObjectCollection& objectCollection, const QByteArray& dependantId) const;
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
};


} // namespace agentinodata


