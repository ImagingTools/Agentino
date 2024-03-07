#include <agentinodata/agentinodata.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState)
{
	ProcessStateEnum processStateEnum;

	switch (processState) {
	case QProcess::Starting:
		processStateEnum.id = "Starting";
		processStateEnum.name = QString("Starting");

		break;

	case QProcess::Running:
		processStateEnum.id = "Running";
		processStateEnum.name = QString("Running");

		break;

	default:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = QString("Not running");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata



