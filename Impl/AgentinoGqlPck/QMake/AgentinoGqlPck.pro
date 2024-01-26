TARGET = AgentinoGqlPck

include($(ACFDIR)/Config/QMake/ComponentConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

LIBS += -L../../../Lib/$$COMPILER_DIR  -limtguigql -limtbase -limtgql -limtgui -limtwidgets -limtlic -limtdb
LIBS += -lservicemanagergql -lservicemanagerdata

include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(ACFDIR)/Config/QMake/AcfStd.pri)
