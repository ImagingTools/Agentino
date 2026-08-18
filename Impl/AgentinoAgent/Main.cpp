// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthInitializer.h>
#include <imtcore/CImtCoreBaseInitializer.h>
#include <imtcore/CImtCoreDeskInitializer.h>
#include <imtcore/CImtCoreLocalizationInitializer.h>
#include <imtcore/CImtCoreStyleInitializer.h>
#include <imtlic/IProductInfo.h>
#include <GeneratedFiles/AgentinoAgent/CAgentinoAgent.h>

// Same product feature tree as AgentinoServer so Topology GetCommands permission
// checks can resolve ChangeService / ViewServices / …
#include "../AgentinoServer/AgentinoFeatures.h"


class CAgentinoAgentResourceInitializer
{
public:
	static void Init()
	{
#ifdef WEB_COMPILE
#ifdef AGENTINO_USE_NEW_WEB
		Q_INIT_RESOURCE(agentWeb);
#else
		Q_INIT_RESOURCE(agentinoqmlWeb);
#endif
#endif
		Q_INIT_RESOURCE(agentinoqml);
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
	CAgentinoAgentResourceInitializer::Init();

	CAgentinoAgent instance;
	auto* productInfoPtr = instance.GetInterface<imtlic::IProductInfo>();
	if (productInfoPtr != nullptr) {
		agentino::FillProduct(*productInfoPtr);
	}

	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
