TARGET = agentgql

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

LIBS += -L../../../Lib/$$COMPILER_DIR  -limtguigql -limtbase -limtauth -limtgui -limtwidgets -limtlic

