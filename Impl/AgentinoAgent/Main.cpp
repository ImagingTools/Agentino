// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
// Qt includes
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

// ACF includes
#include <ibase/IApplication.h>

// ImtCore includes
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
	Q_INIT_RESOURCE(imtbase);



	CAgentinoAgent instance;

	ibase::IApplication* applicationPtr = instance.GetInterface<ibase::IApplication>();
	if (applicationPtr != nullptr){
		return applicationPtr->Execute(argc, argv);
	}

	return -1;
}


