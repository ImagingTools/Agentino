TARGET = AgentGqlPck

include($(ACFDIR)/Config/QMake/ComponentConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

QT += network

LIBS += -L$(IMTCOREDIR)/Lib/$$COMPILER_DIR  -limtguigql -limtbase -limtgql -limtgui -limtwidgets -limtlic -limtdb -limtservice -limtrest -limtclientgql -limtbase
LIBS += -L../../../Lib/$$COMPILER_DIR -lagentinodata -lagentgql -lagentinosdl

include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(ACFDIR)/Config/QMake/AcfStd.pri)
