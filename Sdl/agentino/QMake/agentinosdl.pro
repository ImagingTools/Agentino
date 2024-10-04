TARGET = agentinosdl

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)


# SDL
include($(IMTCOREDIR)/Config/QMake/SdlCodeGenerator.pri)

SDL_GENERATOR_AGENTS_SCHEME_PATH = $$PWD/../1.0/Agents.sdl
SDL_CPP_AGENTS_COMPILER = $$CreateCppQmlSdlTarget(Agents, $$SDL_GENERATOR_AGENTS_SCHEME_PATH)
$${SDL_CPP_AGENTS_COMPILER}.input = SDL_GENERATOR_AGENTS_SCHEME_PATH
QMAKE_EXTRA_COMPILERS += $${SDL_CPP_AGENTS_COMPILER}
