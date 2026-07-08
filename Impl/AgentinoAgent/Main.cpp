// SPDX-License-Identifier: LicenseRef-Agentino-Commercial


// ImtCore includes
#include <imtbase/Init.h>
#include <GeneratedFiles/AgentinoAgent/CAgentinoAgent.h>


int main(int argc, char *argv[])
{
#ifdef WEB_COMPILE
	Q_INIT_RESOURCE(agentinoqmlWeb);
#endif

	Q_INIT_RESOURCE(agentinoqml);

	return Run<CAgentinoAgent, DefaultImtCoreQmlInitializer>(argc, argv);
}


