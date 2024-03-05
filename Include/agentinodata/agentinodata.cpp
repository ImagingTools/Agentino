#include <agentinodata/agentinodata.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState)
{
	ProcessStateEnum processStateEnum;

	switch (processState) {
	case QProcess::Starting:
		processStateEnum.id = "Starting";
		processStateEnum.name = QObject::tr("Starting");

		break;

	case QProcess::Running:
		processStateEnum.id = "Running";
		processStateEnum.name = QObject::tr("Running");

		break;

	default:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = QObject::tr("Not running");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata



