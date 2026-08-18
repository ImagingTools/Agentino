// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
// Qt includes
#include <QtWidgets/QApplication>

// ImtCore includes
#include <imtcore/CApplicationRunner.h>
#include <imtcore/CImtCoreAuthInitializer.h>
#include <imtcore/CImtCoreBaseInitializer.h>
#include <imtcore/CImtCoreLocalizationInitializer.h>
#include <imtcore/CImtCoreStyleInitializer.h>
#include <imtbase/CTreeItemModel.h>
#include <imtqml/CGqlModel.h>
#include <imtqml/CRemoteFileController.h>
#include <imtqml/CQuickApplicationComp.h>

#include <GeneratedFiles/AgentinoClientServer/CAgentinoClientServer.h>


static void InitializeAgentinoClientServerResources()
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
	Q_INIT_RESOURCE(imtlicguiqml);

	ImtCoreInitLocalizationResources();
	ImtCoreInitBaseResources();

	ImtCoreInitStyleResources();
	ImtCoreInitAuthStyleResources();

	ImtCoreInitQmlApplicationCoreResources();
	ImtCoreInitQmlDocumentManagementResources();
	ImtCoreInitAuthQmlResources();

	InitializeImtCoreStyle();
}


int main(int argc, char* argv[])
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	InitializeAgentinoClientServerResources();

	CAgentinoClientServer instance;

	qmlRegisterType<imtbase::CTreeItemModel>("Acf", 1, 0, "TreeItemModel");
	qmlRegisterType<imtqml::CGqlModel>("Acf", 1, 0, "GqlModel");
	qmlRegisterType<imtqml::CRemoteFileController>("Acf", 1, 0, "RemoteFileController");

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	qmlRegisterModule("QtGraphicalEffects", 1, 12);
	qmlRegisterModule("QtGraphicalEffects", 1, 0);
	qmlRegisterModule("QtQuick.Dialogs", 1, 3);
#else
	qmlRegisterModule("QtQuick.Dialogs", 6, 2);
	qmlRegisterModule("Qt5Compat.GraphicalEffects", 6, 0);
#endif

	return imtcore::CApplicationRunner::Run(argc, argv, instance);
}
