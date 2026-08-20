cmake_minimum_required(VERSION 3.26)

project(AgentinoAll)

if(NOT DEFINED QT_VERSION_MAJOR)
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core)
	find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Core Widgets QuickWidgets Core Gui Xml Network Svg Sql WebSockets Qml Concurrent QuickControls2 PrintSupport REQUIRED)
	if(QT_VERSION_MAJOR EQUAL 6)
		find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Core5Compat REQUIRED)
	endif()
endif()

if(NOT DEFINED WEB_COMPILE)
	set(WEB_COMPILE ON CACHE BOOL "Build whith web compatibility")
endif()

if(NOT DEFINED AGENTINODIR_BUILD)
	set(AGENTINODIR_BUILD "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

if(NOT DEFINED IMTCOREDIR)
	set(IMTCOREDIR "$ENV{IMTCOREDIR}")
endif()

if(NOT DEFINED SMDIR)
	set(PUMADIR "$ENV{SMDIR}")
endif()

# Enable modern target-based architecture: find_package(ImtCore) provides ImtCore::
# imported targets (and, transitively, Acf::/AcfSln::/IAcf::) with transitive include
# dirs and link deps. Legacy global include_directories()/link_directories() are skipped.
set(ACF_MODERN_CMAKE ON)

# Collect Agentino libraries in their own export set. Read by acf_register_library() (from Acf).
set(ACF_PACKAGE_NAME "Agentino")

include(${IMTCOREDIR}/Config/CMake/ImtCoreEnv.cmake)

# Discover the ImtCore package published by its build tree (pulls in Acf/AcfSln/IAcf).
if(ACF_MODERN_CMAKE AND NOT TARGET ImtCore::imtbase)
	set(ImtCore_DIR "${IMTCOREDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}/cmake" CACHE PATH "Path to the ImtCore build-tree CMake package")
	find_package(ImtCore REQUIRED GLOBAL)
endif()

if(USE_FIND_PACKAGE)
	find_program(NODE_EXE NAMES node)
	message("NODE_EXE found: ${NODE_EXE}")
endif()

set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Include")
set(IMPL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Impl")
set(SDL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Sdl")

set(AUX_INCLUDE_DIR "${AGENTINODIR_BUILD}/AuxInclude/${TARGETNAME}/GeneratedFiles")

include_directories("${AUX_INCLUDE_DIR}/../")
include_directories("${AUX_INCLUDE_DIR}/")

# Expose dependency helper for per-target CMakeLists.
set(AGENTINO_DECLARE_DEPENDENCIES_HELPER_ONLY ON)
include("${CMAKE_CURRENT_LIST_DIR}/../../Config/CMake/AgentinoLibraryDependencies.cmake")
unset(AGENTINO_DECLARE_DEPENDENCIES_HELPER_ONLY)

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
add_subdirectory("${SDL_DIR}/agentino/CMake" "${AUX_INCLUDE_DIR}/agentinosdl")
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

# Declare target-based deps so include paths/link order propagate transitively across Agentino
# libraries and onto the underlying ImtCore::/Acf::/AcfSln::/IAcf:: libraries.
include("${CMAKE_CURRENT_LIST_DIR}/../../Config/CMake/AgentinoLibraryDependencies.cmake")

