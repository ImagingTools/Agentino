project(agentinosdl)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCoreSdlAutoConfig.cmake)

add_custom_target(${PROJECT_NAME}-SDL-related-files SOURCES
	${CMAKE_CURRENT_LIST_DIR}/../1.0/Agents.sdl)

ImtCoreAutoConfigureSdlCppQml(
			SDL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../1.0/Agents.sdl"
			CUSTOM_TARGETS_DEPS ON
			CUSTOM_HEADER_DEP "1.0/CPP/Agents.h"
			CUSTOM_SOURCE_DEP "1.0/CPP/Agents.cpp"
			CUSTOM_QML_RES_DEP "1.0/QML/agentinoAgentsSdl/agentinoAgentsSdl.qrc")

