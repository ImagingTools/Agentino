#include <agentinodata/agentinodata.h>


namespace agentinodata
{


ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState)
{
	ProcessStateEnum processStateEnum;
	QString internalTranslate;

	switch (processState) {
	case QProcess::Starting:
		processStateEnum.id = "Starting";
		processStateEnum.name = "Starting";
		internalTranslate = QObject::tr("Starting");

		break;

	case QProcess::Running:
		processStateEnum.id = "Running";
		processStateEnum.name = "Running";
		internalTranslate = QObject::tr("Running");

		break;

	default:
		processStateEnum.id = "NotRunning";
		processStateEnum.name = "Not running";
		internalTranslate = QObject::tr("Not running");

		break;
	}

	return processStateEnum;
}



} // namespace agentinodata



