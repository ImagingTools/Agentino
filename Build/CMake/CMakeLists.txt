cmake_minimum_required(VERSION 3.26)

project(Agentino)


option(WEB_COMPILE "Build whith web compatibility" ON)
option(USE_NEW_WEB "Use new web compilation (jq_compile_web)" ON)
option(BUILD_TESTING "Configures projects for testing" ON)

if(NOT DEFINED AGENTINODIR)
	get_filename_component(AGENTINODIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

if(NOT DEFINED SMDIR)
	file(TO_CMAKE_PATH "$ENV{SMDIR}" SMDIR)
endif()

# Enable modern target-based architecture
set(ACF_MODERN_CMAKE ON)

# Collect Agentino libraries in their own export set. Read by acf_register_library() (from Acf).
set(ACF_PACKAGE_NAME "Agentino")

include(${AGENTINODIR}/Config/CMake/AgentinoEnv.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCoreSdlCustomConfig.cmake)

find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Core Widgets QuickWidgets Core Gui Xml Network Svg Sql WebSockets Qml Concurrent QuickControls2 PrintSupport REQUIRED)
if(QT_VERSION_MAJOR EQUAL 6)
	find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Core5Compat REQUIRED)
endif()

if(USE_FIND_PACKAGE)
	find_program(NODE_EXE NAMES node)
	message("NODE_EXE found: ${NODE_EXE}")
endif()

set(INCLUDE_DIR		"${AGENTINODIR}/Include")
set(IMPL_DIR		"${AGENTINODIR}/Impl")
set(SDL_DIR			"${AGENTINODIR}/Sdl")
set(AUX_INCLUDE_DIR "${AGENTINODIR_BUILD}/AuxInclude/${TARGETNAME}/GeneratedFiles")

# SDL
ImtCoreAddSchemaSearchPath("${IMTCOREDIR}/Sdl")

add_subdirectory("${SDL_DIR}/agentino/CMake" "${AUX_INCLUDE_DIR}/agentinosdl")

# Include
add_subdirectory("${INCLUDE_DIR}/agentinodata/CMake" "${AUX_INCLUDE_DIR}/agentinodata")
add_dependencies(agentinodata agentinosdl)

add_subdirectory("${INCLUDE_DIR}/agentinogql/CMake" "${AUX_INCLUDE_DIR}/agentinogql")
add_dependencies(agentinogql agentinosdl agentinodata)

add_subdirectory("${INCLUDE_DIR}/agentgql/CMake" "${AUX_INCLUDE_DIR}/agentgql")
add_dependencies(agentgql agentinosdl)

add_subdirectory("${INCLUDE_DIR}/agentinoqml/CMake" "${AUX_INCLUDE_DIR}/agentinoqml")
add_dependencies(agentinoqml agentinosdl)

# Impl
add_subdirectory("${IMPL_DIR}/AgentinoGqlPck/CMake" "${AUX_INCLUDE_DIR}/AgentinoGqlPck")
add_dependencies(AgentinoGqlPck agentinogql)

add_subdirectory("${IMPL_DIR}/AgentGqlPck/CMake" "${AUX_INCLUDE_DIR}/AgentGqlPck")
add_dependencies(AgentGqlPck agentgql)

add_subdirectory("${IMPL_DIR}/AgentinoDataPck/CMake" "${AUX_INCLUDE_DIR}/AgentinoDataPck")
add_dependencies(AgentinoDataPck agentinodata)

add_subdirectory("${IMPL_DIR}/AgentinoLoc/CMake" "${AUX_INCLUDE_DIR}/AgentinoLoc")
add_subdirectory("${IMPL_DIR}/AgentinoServer/CMake" "${AUX_INCLUDE_DIR}/AgentinoServer")
add_subdirectory("${IMPL_DIR}/AgentinoClient/CMake" "${AUX_INCLUDE_DIR}/AgentinoClient")
add_subdirectory("${IMPL_DIR}/AgentinoAgent/CMake" "${AUX_INCLUDE_DIR}/AgentinoAgent")

if(BUILD_TESTING)
	add_subdirectory("${IMPL_DIR}/AgentinoAgentTest/CMake" "${AUX_INCLUDE_DIR}/AgentinoAgentTest")
	add_subdirectory("${IMPL_DIR}/AgentinoServerTest/CMake" "${AUX_INCLUDE_DIR}/AgentinoServerTest")
endif()

# Declare target-based deps so include paths/link order propagate transitively across Agentino
# libraries and onto the underlying ImtCore::/Acf::/AcfSln::/IAcf:: libraries.
include("${CMAKE_CURRENT_LIST_DIR}/../../Config/CMake/AgentinoLibraryDependencies.cmake")

