TARGET = agentinoqml

include($(ACFDIR)/Config/QMake/GeneralConfig.pri)
include($(IMTCOREDIR)/Config/QMake/QmlControls.pri)

buildwebdir = $$PWD/../../../Bin/web

imtcoredir = $(IMTCOREDIR)

prepareWebQml($$buildwebdir)

# copy DDL files
# copyToWebDir($$PWD/../../../$$AUXINCLUDEDIR/GeneratedFiles/agentinodata/Ddl/Qml/agentino/, $$buildwebdir/src/agentinoDDL)

# copy SDL files
# copyToWebDir($$PWD/../../../$$AUXINCLUDEDIR/GeneratedFiles/agentinogql/SDL/QML, $$buildwebdir/src/agentinoSDL)

# copy project qml from to
copyToWebDir($$PWD/../Qml/, $$buildwebdir/src)
copyToWebDir($$PWD/../Resources/html/, $$buildwebdir/Resources)

# copy translations
copyToWebDir($$PWD/../../../Impl/AgentinoLoc/Translations/, $$buildwebdir/Resources/Translations)
copyToWebDir($$imtcoredir/Impl/ImtCoreLoc/Translations/, $$buildwebdir/Resources/Translations)

copyToWebDir($$imtcoredir/Include/imtstylecontrolsqml/Qml/Fonts/, $$buildwebdir/Resources)
copyToWebDir($$imtcoredir/Include/imtstylecontrolsqml/Qml/Acf/, $$buildwebdir/src/Acf)

copyToWebDir($$imtcoredir/../Agentino/$$AUXINCLUDEDIR/GeneratedFiles/agentinodata/Ddl/Qml/agentino, $$buildwebdir/src/agentino)

copyToWebDir($$imtcoredir/../Agentino/$$AUXINCLUDEDIR/GeneratedFiles/agentinosdl/SDL/1.0/QML/agentinoAgentsSdl, $$buildwebdir/src/agentinoAgentsSdl)

compyleWeb($$buildwebdir, "agentino")

GENERATED_RESOURCES = $$_PRO_FILE_PWD_/../empty

include($(IMTCOREDIR)/Config/QMake/WebQrc.pri)

include($(ACFCONFIGDIR)/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

RESOURCES += $$files($$_PRO_FILE_PWD_/../*.qrc, false)

