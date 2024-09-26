TARGET = AgentinoGqlPck

include($(ACFDIR)/Config/QMake/ComponentConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

QT += network

LIBS += -L../../../Lib/$$COMPILER_DIR  -limtguigql -limtbase -limtgql -limtgui -limtwidgets -limtlic -limtdb -limtservice
LIBS += -limtbase
LIBS += -lagentinogql -lagentinodata -lagentgql -limtclientgql -limtrest

include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(ACFDIR)/Config/QMake/AcfStd.pri)
