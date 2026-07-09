// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// ImtCore includes
#include <imtbase/Init.h>
#include <GeneratedFiles/AgentinoAgent/CAgentinoAgent.h>


int main(int argc, char *argv[])
{
#ifdef WEB_COMPILE
	Q_INIT_RESOURCE(agentinoqmlWeb);
#endif
	Q_INIT_RESOURCE(imtstyle);
	Q_INIT_RESOURCE(imtstylecontrolsqml);
	Q_INIT_RESOURCE(imtauthguiqml);
	Q_INIT_RESOURCE(imtguigqlqml);
	Q_INIT_RESOURCE(imtcontrolsqml);
	Q_INIT_RESOURCE(imtgui);
	Q_INIT_RESOURCE(imtguiqml);
	Q_INIT_RESOURCE(imtdocguiqml);
	Q_INIT_RESOURCE(imtcolguiqml);
	Q_INIT_RESOURCE(agentinoqml);
	Q_INIT_RESOURCE(ImtCoreLoc);
	// Q_INIT_RESOURCE(AgentinoLoc);
	Q_INIT_RESOURCE(imtauthguiTheme);
	Q_INIT_RESOURCE(imtguiTheme);
	Q_INIT_RESOURCE(imtdb);
	Q_INIT_RESOURCE(imtauthdb);
	Q_INIT_RESOURCE(imtbase);

	return Run<CAgentinoAgent, DefaultImtCoreQmlInitializer>(argc, argv);
}
