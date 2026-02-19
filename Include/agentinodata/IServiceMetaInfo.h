// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{

/**
	Interface for describing an service dependencies.
	\ingroup Service
*/
class IServiceMetaInfo: virtual public istd::IChangeable
{
public:
	struct Dependency
	{
		QByteArray typeId;
	};

	virtual QList<Dependency> GetDependensies() const = 0;
};


} // namespace agentinodata




