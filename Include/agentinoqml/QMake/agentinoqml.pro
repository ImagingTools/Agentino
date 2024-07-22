TARGET = agentinoqml

include($(IMTCOREDIR)/Config/QMake/QmlControls.pri)

buildwebdir = $$PWD/../../../Bin/web

imtcoredir = $(IMTCOREDIR)

prepareWebQml($$buildwebdir)

# copy project qml from to
copyToWebDir($$PWD/../Qml/, $$buildwebdir/src)
copyToWebDir($$PWD/../Resources/html/, $$buildwebdir/Resources)
copyToWebDir($$imtcoredir/Include/imtstylecontrolsqml/Qml/Fonts/, $$buildwebdir/Resources)
copyToWebDir($$imtcoredir/Include/imtstylecontrolsqml/Qml/Acf/, $$buildwebdir/src/Acf)

compyleWeb($$buildwebdir, "agentino")

GENERATED_RESOURCES = $$_PRO_FILE_PWD_/../empty

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

RESOURCES += $$files($$_PRO_FILE_PWD_/../*.qrc, false)
