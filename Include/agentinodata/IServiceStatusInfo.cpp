#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(IServiceStatusInfo::ServiceStatus processState)
{
	ProcessStateEnum processStateEnum;

	switch (processState){
	case IServiceStatusInfo::SS_STARTING:
		processStateEnum.id = "starting";
		processStateEnum.name = QString("Starting");

		break;
	case IServiceStatusInfo::SS_STOPPING:
		processStateEnum.id = "stopping";
		processStateEnum.name = QString("Stopping");

		break;

	case IServiceStatusInfo::SS_RUNNING:
		processStateEnum.id = "running";
		processStateEnum.name = QString("Running");

		break;

	case IServiceStatusInfo::SS_NOT_RUNNING:
		processStateEnum.id = "notRunning";
		processStateEnum.name = QString("Not running");

		break;

	default:
		processStateEnum.id = "undefined";
		processStateEnum.name = QString("Undefined");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata


