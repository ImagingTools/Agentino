TARGET = agentinoqml

include($(ACFDIR)/Config/QMake/GeneralConfig.pri)
include($(IMTCOREDIR)/Config/QMake/WebCompiler.pri)

INCLUDEPATH += $$AUXINCLUDEPATH/GeneratedFiles

buildwebdir = $$PWD/../../../Bin/web

imtcoredir = $$(IMTCOREDIR)

# root of the build tree, used by agentino.json (BUILD_FOLDER) to locate generation_info.json of the generated SDL QML modules
buildfolder = $$clean_path($$OUT_PWD/../../..)
win32{
	buildfolder ~= s,/,\\,g
	WEB_COMMAND = set \"BUILD_FOLDER=$$buildfolder\"
}
else{
	WEB_COMMAND = export BUILD_FOLDER=$$shell_quote($$buildfolder)
}

# compile web application with the JQML v3 compiler, QML sources are taken from the directories listed in agentino.json
jqCompileWeb($$buildwebdir, $$PWD/../agentino.json, $$PWD/../Qml/AgentinoWeb.qml, "/Agentino/Views/", "../Icons/AgentinoIcon.svg", $$PWD/../../../Impl/AgentinoLoc/Translations $$imtcoredir/Impl/ImtCoreLoc/Translations)

GENERATED_RESOURCES = $$_PRO_FILE_PWD_/../empty

include($(IMTCOREDIR)/Config/QMake/WebQrc.pri)

include($(ACFCONFIGDIR)/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

RESOURCES += $$files($$_PRO_FILE_PWD_/../*.qrc, false)

