// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthorizableServerInitializer.h>
#include <imtcore/CImtCoreDeskInitializer.h>
#include <imtlic/IProductInfo.h>

// Agentino includes
#include <GeneratedFiles/AgentinoServer/CAgentinoServer.h>
#include "AgentinoFeatures.h"


class CAgentinoServerResourceInitializer
{
public:
	static void Init()
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

		InitializeImtCoreAuthorizableServer();
		ImtCoreInitDeskSqlResources();
	}
};


int main(int argc, char* argv[])
{
	CAgentinoServerResourceInitializer::Init();

	CAgentinoServer instance;
	auto* productInfoPtr = instance.GetInterface<imtlic::IProductInfo>();
	if (productInfoPtr != nullptr) {
		agentino::FillProduct(*productInfoPtr);
	}

	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
