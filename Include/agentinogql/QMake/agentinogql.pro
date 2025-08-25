TARGET = agentinogql

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

# SDL
SDL_SCHEMES_LIST = $$PWD/../1.0/APIv1_0.sdl

include($(IMTCOREDIR)/Config/QMake/SdlConfiguration.pri)