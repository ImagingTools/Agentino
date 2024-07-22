TARGET = agentinogql

include($(ACFDIR)/Config/QMake/StaticConfig.pri)
include($(ACFDIR)/Config/QMake/AcfQt.pri)
include($(IMTCOREDIR)/Config/QMake/ImtCore.pri)

# SDL
SDL_GENERATOR_SCHEME_PATH = $$PWD/../SDL/APIv1_0.sdl
SDL_GENERATOR_CPP_OUT_SUBFOLDER = $$OUT_PWD/$$AUXINCLUDEPATH/GeneratedFiles/$$TARGET/SDL/CPP
SDL_GENERATOR_CPP_NAMESPACE = "$$TARGET::sdl"
SDL_GENERATOR_QML_OUT_SUBFOLDER = $$OUT_PWD/$$AUXINCLUDEPATH/GeneratedFiles/$$TARGET/SDL/QML
SDL_GENERATOR_QML_MODULE_NAME = $${TARGET}Sdl
include($(IMTCOREDIR)/Config/QMake/SdlCodeGenerator.pri)

LIBS += -L../../../Lib/$$COMPILER_DIR  -limtguigql -limtbase -limtauth -limtgui -limtwidgets -limtlic

