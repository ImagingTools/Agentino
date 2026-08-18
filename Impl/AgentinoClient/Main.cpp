// SPDX-License-Identifier: LicenseRef-Agentino-Commercial


// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthInitializer.h>
#include <imtcore/CImtCoreBaseInitializer.h>
#include <imtcore/CImtCoreLocalizationInitializer.h>
#include <imtcore/CImtCoreStyleInitializer.h>

// Agentino includes
#include <GeneratedFiles/AgentinoClient/CAgentinoClient.h>


static void InitializeAgentinoClientResources()
{
	Q_INIT_RESOURCE(AgentinoLoc);
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(agentinoAgentsSdl);
	Q_INIT_RESOURCE(agentinoServicesSdl);
	Q_INIT_RESOURCE(agentinoTopologySdl);
	Q_INIT_RESOURCE(agentinoEnrollmentSdl);
	Q_INIT_RESOURCE(agentino);

	ImtCoreInitLocalizationResources();
	ImtCoreInitBaseResources();

	ImtCoreInitStyleResources();
	ImtCoreInitAuthStyleResources();

	ImtCoreInitQmlApplicationCoreResources();
	ImtCoreInitQmlDocumentManagementResources();
	ImtCoreInitAuthQmlResources();

	InitializeImtCoreStyle();
}


int main(int argc, char* argv[])
{
	InitializeAgentinoClientResources();

	CAgentinoClient instance;
	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
