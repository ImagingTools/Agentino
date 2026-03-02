TARGET = AgentGqlPck

include($(ACFDIR)/Config/QMake/ComponentConfig.pri)
include($(AGENTINODIR)/Config/QMake/Agentino.pri)

QT += network

LIBS += -L../../../Lib/$$COMPILER_DIR -limtbase -limtrest -limtservergql -limtserverapp

include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(ACFDIR)/Config/QMake/AcfStd.pri)
