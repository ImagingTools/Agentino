TARGET = agentinogql

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)


INCLUDEPATH += $$AUXINCLUDEPATH/GeneratedFiles

LIBS += -L../../../Lib/$$COMPILER_DIR -l-lagentinodata
