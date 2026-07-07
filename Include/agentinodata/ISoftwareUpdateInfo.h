// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <iser/IObject.h>

// Qt includes
#include <QString>
#include <QByteArray>


namespace agentinodata
{


/**
	Interface for describing an update info.
	\ingroup Updates
*/
class ISoftwareUpdateInfo: virtual public iser::IObject
{
public:
	/**
		Supported update types.
	*/
	enum UpdateType
	{
		/**
			Agent update.
		*/
		UT_AGENT,

		/**
			Service update.
		*/
		UT_SERVICE
	};

	/**
		Update status values.
	*/
	enum UpdateStatus
	{
		/**
			Update is available for download.
		*/
		US_AVAILABLE,

		/**
			Update is being downloaded.
		*/
		US_DOWNLOADING,

		/**
			Update is being installed.
		*/
		US_INSTALLING,

		/**
			Update has been installed.
		*/
		US_INSTALLED,

		/**
			Update installation failed.
		*/
		US_FAILED,

		/**
			Update has been rolled back.
		*/
		US_ROLLED_BACK
	};

	/**
		Get name of the update.
	*/
	virtual QString GetName() const = 0;

	/**
		Get version of the update.
	*/
	virtual QString GetVersion() const = 0;

	/**
		Get type of the update (Agent or Service).
	*/
	virtual UpdateType GetUpdateType() const = 0;

	/**
		Get description of the update.
	*/
	virtual QString GetDescription() const = 0;

	/**
		Get file size of the update artifact in bytes.
	*/
	virtual qint64 GetFileSize() const = 0;

	/**
		Get checksum of the update artifact.
	*/
	virtual QString GetChecksum() const = 0;

	/**
		Get published date of the update.
	*/
	virtual QString GetPublishedDate() const = 0;

	/**
		Get current status of the update.
	*/
	virtual UpdateStatus GetStatus() const = 0;

	/**
		Get changelog information.
	*/
	virtual QString GetChangelog() const = 0;

	/**
		Get target version this update applies to.
	*/
	virtual QString GetTargetVersion() const = 0;

	/**
		Get source URL for downloading the update artifact.
	*/
	virtual QString GetSourceUrl() const = 0;
};


} // namespace agentinodata


