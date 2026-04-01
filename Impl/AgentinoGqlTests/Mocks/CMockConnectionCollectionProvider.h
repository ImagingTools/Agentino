// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once

// Qt includes
#include <QtCore/QMap>

// ImtCore includes
#include <imtservice/IConnectionCollectionProvider.h>


namespace agentinotest
{

/**
	Mock implementation of IConnectionCollectionProvider for testing service controllers.
*/
class CMockConnectionCollectionProvider: virtual public imtservice::IConnectionCollectionProvider
{
public:
	CMockConnectionCollectionProvider() = default;

	// reimplemented (imtservice::IConnectionCollectionProvider)
	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServicePath(const QString& /*servicePath*/) const override
	{
		return imtservice::IConnectionCollectionSharedPtr();
	}

	virtual imtservice::IConnectionCollectionSharedPtr GetConnectionCollectionByServiceId(const QByteArray& /*serviceId*/) const override
	{
		return imtservice::IConnectionCollectionSharedPtr();
	}
};

} // namespace agentinotest
