// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IPolymorphic.h>

// Qt includes
#include <QByteArray>
#include <QString>


namespace agentinodata
{


/**
	Interface for managing software updates.
	Handles downloading, installing, and rolling back updates.
	\ingroup Updates
*/
class IUpdateManager: virtual public istd::IPolymorphic
{
public:
	/**
		Result of an update operation.
	*/
	struct UpdateResult
	{
		bool successful = false;
		int status = 0;
		QString errorMessage;
	};

	/**
		Apply an update to the specified agent.
		Downloads and installs the update artifact.
		\param updateId ID of the update to apply.
		\param agentId ID of the target agent.
		\return Result of the apply operation.
	*/
	virtual UpdateResult ApplyUpdate(const QByteArray& updateId, const QByteArray& agentId) = 0;

	/**
		Rollback a previously installed update.
		\param updateId ID of the update to rollback.
		\param agentId ID of the target agent.
		\return Result of the rollback operation.
	*/
	virtual UpdateResult RollbackUpdate(const QByteArray& updateId, const QByteArray& agentId) = 0;
};


} // namespace agentinodata


