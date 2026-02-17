// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtlic/Init.h>

// Agentino includes
#include <GeneratedFiles/AgentinoServer/CAgentinoServer.h>
#include "AgentinoFeatures.h"


int main(int argc, char *argv[])
{
#ifdef WEB_COMPILE
	Q_INIT_RESOURCE(agentinoqmlWeb);
#endif
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(AgentinoLoc);
	Q_INIT_RESOURCE(agentino);

	return ProductFeatureRun<CAgentinoServer, DefaultImtCoreQmlInitializer, agentino::FillProduct>(argc, argv);
}


