// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthInitializer.h>
#include <imtcore/CImtCoreBaseInitializer.h>
#include <imtcore/CImtCoreDeskInitializer.h>
#include <imtcore/CImtCoreLocalizationInitializer.h>
#include <imtcore/CImtCoreStyleInitializer.h>

// Agentino includes
#include <GeneratedFiles/AgentinoClient/CAgentinoClient.h>


class CAgentinoClientResourceInitializer
{
public:
	static void Init()
	{
		Q_INIT_RESOURCE(AgentinoLoc);
		Q_INIT_RESOURCE(agentinoqml);
		Q_INIT_RESOURCE(agentinoAgentsSdl);
		Q_INIT_RESOURCE(agentinoServicesSdl);
		Q_INIT_RESOURCE(agentinoTopologySdl);
		Q_INIT_RESOURCE(agentinoEnrollmentSdl);
		Q_INIT_RESOURCE(agentino);
		Q_INIT_RESOURCE(imtlicguiqml);

		ImtCoreInitLocalizationResources();
		ImtCoreInitBaseResources();
		ImtCoreInitAuthSqlResources();
		ImtCoreInitDeskSqlResources();

		ImtCoreInitStyleResources();
		ImtCoreInitAuthStyleResources();

		ImtCoreInitQmlApplicationCoreResources();
		ImtCoreInitQmlDocumentManagementResources();
		ImtCoreInitAuthQmlResources();

		InitializeImtCoreStyle();
	}
};


int main(int argc, char* argv[])
{
	CAgentinoClientResourceInitializer::Init();

	CAgentinoClient instance;
	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
