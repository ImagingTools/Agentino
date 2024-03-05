#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(IServiceStatusInfo::ServiceStatus processState)
{
	ProcessStateEnum processStateEnum;
	QString internalTranslate;

	switch (processState) {
	case IServiceStatusInfo::SS_STARTING:
		processStateEnum.id = "Starting";
		processStateEnum.name = QObject::tr("Starting");

		break;

	case IServiceStatusInfo::SS_RUNNING:
		processStateEnum.id = "Running";
		processStateEnum.name = QObject::tr("Running");

		break;

	case IServiceStatusInfo::SS_NOT_RUNNING:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = QObject::tr("Not running");

		break;

	default:
		processStateEnum.id = "Indefined";
		processStateEnum.name = QObject::tr("Indefined");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata


