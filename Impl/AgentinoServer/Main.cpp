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


