TARGET = agentinodata

include($(ACFCONFIGDIR)/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

MODULE_CPP_NAME = agentino
MODULE_QML_NAME = agentino
DDL_TEMPLATE_INPUT_DIR = $$PWD/../Resources/Ddl
DDL_OUTPUT_DIR = $$OUT_PWD/$$AUXINCLUDEPATH/GeneratedFiles/$$TARGET/Ddl

include($(IMTCOREDIR)/Config/QMake/DdlCodeCreator.pri)


