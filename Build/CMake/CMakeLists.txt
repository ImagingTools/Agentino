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

acf_define_link_scope_var(ACF_QT_MODULE_LINK_SCOPE  "PRIVATE" "Link scope used for Qt module dependencies")
acf_define_link_scope_var(ACF_LIBRARY_LINK_SCOPE     "PUBLIC"  "Link scope used by Agentino inter-library dependencies")
acf_define_link_scope_var(ACF_PACKAGE_LINK_SCOPE     "PRIVATE" "Link scope used by package (Pck) libraries linking their dependencies")
acf_define_link_scope_var(ACF_APPLICATION_LINK_SCOPE "PRIVATE" "Link scope used by executables linking their dependencies")

function(agentino_create_legacy_short_target_aliases)
	get_property(_all_targets GLOBAL PROPERTY TARGETS)
	foreach(_t IN LISTS _all_targets)
		if(_t MATCHES "^(Acf|AcfSln|IAcf|ImtCore)::(.+)$")
			set(_alias "${CMAKE_MATCH_2}")
			if(NOT TARGET ${_alias})
				get_target_property(_type ${_t} TYPE)
				if(_type STREQUAL "STATIC_LIBRARY" OR _type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "INTERFACE_LIBRARY")
					add_library(${_alias} ALIAS ${_t})
				endif()
			endif()
		endif()
	endforeach()
endfunction()

# Legacy ACF compatibility: if helper CMake files still use plain
# target_link_libraries() signatures, force plain mode in this repository too.
set(AGENTINO_FORCE_PLAIN_TLL "AUTO" CACHE STRING "Force plain target_link_libraries signatures: AUTO|ON|OFF")
set_property(CACHE AGENTINO_FORCE_PLAIN_TLL PROPERTY STRINGS "AUTO" "ON" "OFF")

set(_agentino_force_plain_tll OFF)
if(AGENTINO_FORCE_PLAIN_TLL STREQUAL "ON")
	set(_agentino_force_plain_tll ON)
elseif(AGENTINO_FORCE_PLAIN_TLL STREQUAL "AUTO")
	set(_acf_qt_cmake "${ACFDIR}/Config/CMake/AcfQt.cmake")
	if(EXISTS "${_acf_qt_cmake}")
		file(READ "${_acf_qt_cmake}" _acf_qt_cmake_text)
		string(FIND "${_acf_qt_cmake_text}" "ACF_LIBRARY_LINK_SCOPE" _acf_qt_uses_scope)
		if(_acf_qt_uses_scope EQUAL -1)
			set(_agentino_force_plain_tll ON)
		endif()
	endif()
endif()

if(_agentino_force_plain_tll)
	set(ACF_QT_MODULE_LINK_SCOPE "" CACHE STRING "Link scope used for Qt module dependencies" FORCE)
	set(ACF_LIBRARY_LINK_SCOPE "" CACHE STRING "Link scope used by Agentino inter-library dependencies" FORCE)
	set(ACF_PACKAGE_LINK_SCOPE "" CACHE STRING "Link scope used by package (Pck) libraries linking their dependencies" FORCE)
	set(ACF_APPLICATION_LINK_SCOPE "" CACHE STRING "Link scope used by executables linking their dependencies" FORCE)

	# In ACF_MODERN_CMAKE mode global link_directories() are disabled, but
	# legacy Acf*.cmake modules still add plain names like "iqtdoc".
	if(DEFINED ACFDIR_BUILD AND NOT ACFDIR_BUILD STREQUAL "")
		link_directories("${ACFDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED ACFDIR AND NOT ACFDIR STREQUAL "")
		link_directories("${ACFDIR}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED ACFSLNDIR_BUILD AND NOT ACFSLNDIR_BUILD STREQUAL "")
		link_directories("${ACFSLNDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED ACFSLNDIR AND NOT ACFSLNDIR STREQUAL "")
		link_directories("${ACFSLNDIR}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED IACFDIR_BUILD AND NOT IACFDIR_BUILD STREQUAL "")
		link_directories("${IACFDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED IACFDIR AND NOT IACFDIR STREQUAL "")
		link_directories("${IACFDIR}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED IMTCOREDIR_BUILD AND NOT IMTCOREDIR_BUILD STREQUAL "")
		link_directories("${IMTCOREDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()
	if(DEFINED IMTCOREDIR AND NOT IMTCOREDIR STREQUAL "")
		link_directories("${IMTCOREDIR}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}")
	endif()

	# Legacy Acf*.cmake modules use short names; create aliases so CMake can
	# resolve them to known imported targets when available.
	agentino_create_legacy_short_target_aliases()
endif()

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

