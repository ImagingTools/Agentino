// SPDX-License-Identifier: LicenseRef-Agentino-Commercial


// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthorizableServerInitializer.h>
#include <imtcore/CImtCoreDeskInitializer.h>
#include <imtcore/CImtCoreLicInitializer.h>
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
	Q_INIT_RESOURCE(imtstylecontrolsqml);

	ImtCoreInitDeskSqlResources();
	InitializeImtCoreAuthorizableServer();

	ImtCoreInitStyleResources();
	ImtCoreInitAuthStyleResources();
	ImtCoreInitLicStyleResources();
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
