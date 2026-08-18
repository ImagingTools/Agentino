// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthInitializer.h>
#include <imtcore/CImtCoreLocalizationInitializer.h>
#include <imtlic/IProductInfo.h>

// Agentino includes
#include <GeneratedFiles/AgentinoServer/CAgentinoServer.h>
#include "AgentinoFeatures.h"


static void InitializeAgentinoServerResources()
{
#ifdef WEB_COMPILE
#ifdef AGENTINO_USE_NEW_WEB
	Q_INIT_RESOURCE(agentinoWeb);
#else
	Q_INIT_RESOURCE(agentinoqmlWeb);
#endif
#endif
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(AgentinoLoc);
	Q_INIT_RESOURCE(agentino);

	ImtCoreInitLocalizationResources();
	ImtCoreInitAuthSqlResources();
}


int main(int argc, char* argv[])
{
	InitializeAgentinoServerResources();

	CAgentinoServer instance;
	auto* productInfoPtr = instance.GetInterface<imtlic::IProductInfo>();
	if (productInfoPtr != nullptr){
		agentino::FillProduct(*productInfoPtr);
	}

	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
