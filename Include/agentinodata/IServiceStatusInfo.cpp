// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
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

	case IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
		processStateEnum.id = "runningImpossible";
		processStateEnum.name = QString("Running impossible");

		break;

	default:
		processStateEnum.id = "undefined";
		processStateEnum.name = QString("Undefined");

		break;
	}

	return processStateEnum;
}


bool GetServiceStatusFromRepresentation(
			const QString& representation,
			IServiceStatusInfo::ServiceStatus& processState)
{
	const QString normalized = representation.trimmed();
	if (normalized.isEmpty()){
		return false;
	}

	// I_DECLARE_ENUM names ("SS_RUNNING", ...) as emitted by EmitChangeSignal.
	IServiceStatusInfo::ServiceStatus parsed = IServiceStatusInfo::SS_UNDEFINED;
	if (IServiceStatusInfo::FromString(normalized.toUtf8(), parsed)){
		processState = parsed;

		return true;
	}

	// ProcessStateEnum ids ("running", "notRunning"), snake/upper wire ("NOT_RUNNING", "RUNNING").
	const QString compact = normalized.toLower().remove(QLatin1Char('_')).remove(QLatin1Char(' '));
	if (compact == QStringLiteral("running")){
		processState = IServiceStatusInfo::SS_RUNNING;

		return true;
	}
	if (compact == QStringLiteral("notrunning") || compact == QStringLiteral("stopped")){
		processState = IServiceStatusInfo::SS_NOT_RUNNING;

		return true;
	}
	if (compact == QStringLiteral("starting")){
		processState = IServiceStatusInfo::SS_STARTING;

		return true;
	}
	if (compact == QStringLiteral("stopping")){
		processState = IServiceStatusInfo::SS_STOPPING;

		return true;
	}
	if (compact == QStringLiteral("runningimpossible")){
		processState = IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE;

		return true;
	}
	if (compact == QStringLiteral("undefined")){
		processState = IServiceStatusInfo::SS_UNDEFINED;

		return true;
	}

	return false;
}


} // namespace agentinodata


