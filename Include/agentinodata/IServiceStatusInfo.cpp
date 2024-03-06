#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(IServiceStatusInfo::ServiceStatus processState)
{
	ProcessStateEnum processStateEnum;

	switch (processState) {
	case IServiceStatusInfo::SS_STARTING:
		processStateEnum.id = "Starting";
		processStateEnum.name = QString("Starting");

		break;
	case IServiceStatusInfo::SS_STOPPING:
		processStateEnum.id = "Stopping";
		processStateEnum.name = QString("Stopping");

		break;

	case IServiceStatusInfo::SS_RUNNING:
		processStateEnum.id = "Running";
		processStateEnum.name = QString("Running");

		break;

	case IServiceStatusInfo::SS_NOT_RUNNING:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = QString("Not running");

		break;

	default:
		processStateEnum.id = "Undefined";
		processStateEnum.name = QString("Undefined");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata


