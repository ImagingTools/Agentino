TARGET = AgentinoGqlPck

include($(ACFDIR)/Config/QMake/ComponentConfig.pri)
include($(AGENTINODIR)/Config/QMake/Agentino.pri)

QT += network

LIBS += -L$(IMTCOREDIR)/Lib/$$COMPILER_DIR -limtbase -limtrest -limtserverapp

include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(ACFDIR)/Config/QMake/AcfStd.pri)
