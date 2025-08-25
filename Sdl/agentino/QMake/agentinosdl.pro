TARGET = agentinosdl

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)


# SDL
SDL_SCHEMES_LIST = $$PWD/../1.0/Agents.sdl \
					$$PWD/../1.0/Services.sdl \
					$$PWD/../1.0/Topology.sdl \
					$$PWD/../1.0/ServiceLog.sdl

include($(IMTCOREDIR)/Config/QMake/SdlConfiguration.pri)