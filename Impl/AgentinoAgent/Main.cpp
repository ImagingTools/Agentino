// SPDX-License-Identifier: LicenseRef-Agentino-Commercial


// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthorizableServerInitializer.h>
#include <imtcore/CImtCoreDeskInitializer.h>
#include <imtcore/CImtCoreLicInitializer.h>
#include <imtlic/IProductInfo.h>


// Generated includes
#include <GeneratedFiles/AgentinoAgent/CAgentinoAgent.h>


// Same product feature tree as AgentinoServer so Topology GetCommands permission
// checks can resolve ChangeService / ViewServices / …
#include "../AgentinoServer/AgentinoFeatures.h"


static void InitializeAgentinoAgentResources()
{
#ifdef WEB_COMPILE
#ifdef AGENTINO_USE_NEW_WEB
		Q_INIT_RESOURCE(agentWeb);
#else
		Q_INIT_RESOURCE(agentinoqmlWeb);
#endif
#endif
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(imtstylecontrolsqml);

	ImtCoreInitDeskSqlResources();
	InitializeImtCoreAuthorizableServer();

	ImtCoreInitStyleResources();
	ImtCoreInitAuthStyleResources();
	ImtCoreInitLicStyleResources();
}


int main(int argc, char* argv[])
{
	InitializeAgentinoAgentResources();

	CAgentinoAgent instance;
	auto* productInfoPtr = instance.GetInterface<imtlic::IProductInfo>();
	if (productInfoPtr != nullptr){
		agentino::FillProduct(*productInfoPtr);
	}

	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
