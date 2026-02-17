// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
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




