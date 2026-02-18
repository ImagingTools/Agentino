// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtbase/Init.h>

// Agentino includes
#include <GeneratedFiles/AgentinoClient/CAgentinoClient.h>


int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(AgentinoLoc);
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(agentinoAgentsSdl);
	Q_INIT_RESOURCE(agentinoServicesSdl);
	Q_INIT_RESOURCE(agentinoTopologySdl);
	Q_INIT_RESOURCE(agentino);
	Q_INIT_RESOURCE(imtbase);

	return Run<CAgentinoClient, DefaultImtCoreQmlInitializer>(argc, argv);
}


