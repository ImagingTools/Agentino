include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

INCLUDEPATH += $(AGENTINODIR)/Include
INCLUDEPATH += $(AGENTINODIR)/Impl
INCLUDEPATH += $(AGENTINODIR)/$$AUXINCLUDEDIR

include($(ACFDIR)/Config/QMake/QtBaseConfig.pri)
include($(IMTCOREDIR)/Config/QMake/OpenSSL.pri)
include($(IACFDIR)/Config/QMake/zlib.pri)

HEADERS =
QT += xml network sql quick qml websockets

RESOURCES += $$files($$_PRO_FILE_PWD_/../*.qrc, false)

DEFINES += WEB_COMPILE

LIBS += -L$(ACFDIR)/Lib/$$COMPILER_DIR -lAcfLoc
LIBS += -L$(ACFSLNDIR)/Lib/$$COMPILER_DIR -liauth -liservice -lAcfSlnLoc
LIBS += -L$(IMTCOREDIR)/Lib/$$COMPILER_DIR -limtbase -limtauth -limtauthgui -limtgui -limtlicdb -limtlic -limtlicgui -lImtCoreLoc -limtwidgets -limtzip -limtrest -limtcrypt -limtrepo -limtstyle -limtqml -limtdb -limtfile -limtlog -limtauthsdl -limtappsdl
LIBS += -L$(IMTCOREDIR)/Lib/$$COMPILER_DIR -limtlicgql -limtguigql -limtgql -limtauthgql -limtauthdb -limtcom -limtapp -limtclientgql -limtservice
LIBS += -L$(IMTCOREDIR)/Lib/$$COMPILER_DIR -limtcontrolsqml -limtstylecontrolsqml -limtguigqlqml -limtcolguiqml -limtdocguiqml -limtauthguiqml -limtlicguiqml -limtguiqml -lImtCoreLoc -limtservergql -limtbasesdl -limtcol -limtmail
LIBS += -L$(AGENTINODIR)/Lib/$$COMPILER_DIR -lagentinoqml -lagentinogql -lagentgql -lagentinodata -lagentinosdl -lAgentinoLoc
