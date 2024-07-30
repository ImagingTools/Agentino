cmake_minimum_required(VERSION 3.1.0)

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

if(NOT DEFINED IMTCOREDIR)
	set(IMTCOREDIR "$ENV{IMTCOREDIR}")
endif()

if(NOT DEFINED SMDIR)
	set(PUMADIR "$ENV{SMDIR}")
endif()

include(${IMTCOREDIR}/Config/CMake/ImtCoreEnv.cmake)

set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Include")
set(IMPL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Impl")

set(AUX_INCLUDE_DIR "${BUILDDIR}/Agentino/AuxInclude/${TARGETNAME}/GeneratedFiles")

include_directories("${AUX_INCLUDE_DIR}/../")

# Include
add_subdirectory("${INCLUDE_DIR}/agentinodata/CMake" "${AUX_INCLUDE_DIR}/agentinodata")
add_subdirectory("${INCLUDE_DIR}/agentinogql/CMake" "${AUX_INCLUDE_DIR}/agentinogql")
add_subdirectory("${INCLUDE_DIR}/agentgql/CMake" "${AUX_INCLUDE_DIR}/agentgql")
add_subdirectory("${INCLUDE_DIR}/agentinoqml/CMake" "${AUX_INCLUDE_DIR}/agentinoqml")
add_dependencies(agentinoqml agentinogql agentgql)


# Impl
add_subdirectory("${IMPL_DIR}/AgentinoGqlPck/CMake" "${AUX_INCLUDE_DIR}/AgentinoGqlPck")
add_subdirectory("${IMPL_DIR}/AgentinoDataPck/CMake" "${AUX_INCLUDE_DIR}/AgentinoDataPck")
add_subdirectory("${IMPL_DIR}/AgentinoLoc/CMake" "${AUX_INCLUDE_DIR}/AgentinoLoc")

# add_subdirectory("${IMPL_DIR}/AgentinoClientServer/CMake" "${AUX_INCLUDE_DIR}/AgentinoClientServer")
add_subdirectory("${IMPL_DIR}/AgentinoServer/CMake" "${AUX_INCLUDE_DIR}/AgentinoServer")
add_subdirectory("${IMPL_DIR}/AgentinoClient/CMake" "${AUX_INCLUDE_DIR}/AgentinoClient")
add_subdirectory("${IMPL_DIR}/AgentinoAgent/CMake" "${AUX_INCLUDE_DIR}/AgentinoAgent")
#add_subdirectory("${IMPL_DIR}/AgentinoDataPck/CMake" "${AUX_INCLUDE_DIR}/AgentinoDataPck")
#add_subdirectory("${IMPL_DIR}/AgentinoServerConfigurator/CMake" "${AUX_INCLUDE_DIR}/AgentinoServerConfigurator")

