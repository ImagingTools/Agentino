project(AgentinoClientServer LANGUAGES CXX)

message(VERBOSE "ACFDIR LISA " ${ACFDIR})
message(VERBOSE "CMAKE_PROJECT_INCLUDE_BEFORE " ${CMAKE_PROJECT_INCLUDE_BEFORE})

set(ARXC_CONFIG "${PROJECT_SOURCE_DIR}/../../../Config/Agentino.awc")
set(ARXC_FILES "${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}.acc")
set(ARXC_OUTDIR "${AUX_INCLUDE_DIR}")

message(VERBOSE "ARXC_CONFIG " ${ARXC_CONFIG})
message(VERBOSE "ARXC_OUTDIR " ${ARXC_OUTDIR})

if(WIN32)
	set(CMAKE_WIN32_EXECUTABLE "on")
	set(ACF_CONVERT_FILES "${PROJECT_SOURCE_DIR}/../VC/AgentinoClientServer.rc.xtracf")
	set(ACF_CONVERT_REGISTRY "${PROJECT_SOURCE_DIR}/../VC/FileSubstitCopyApp.acc")
	set(ACF_CONVERT_CONFIG  "${PROJECT_SOURCE_DIR}/../../../Config/BaseOnly.awc")
	set(RC_FILE "${AUX_INCLUDE_DIR}/${PROJECT_NAME}/AgentinoClientServer.rc")
endif()


if(APPLE)
	set(CMAKE_MACOSX_BUNDLE "on")
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebSockets QuickWidgets)
find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Quick Qml REQUIRED)


include(${ACFDIR}/Config/CMake/CustomBuild.cmake)
include(${ACFDIR}/Config/CMake/ApplicationConfig.cmake)
include(${ACFDIR}/Config/CMake/WindeployQt.cmake)

if(DEFINED WEB_COMPILE AND  WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
endif()

if(DEFINED USE_NEW_WEB AND USE_NEW_WEB)
	add_compile_definitions(AGENTINO_USE_NEW_WEB)
endif()

if(DEFINED USE_NEW_WEB AND USE_NEW_WEB)
	add_compile_definitions(AGENTINO_USE_NEW_WEB)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	agentinoqml
	agentinogql
	agentgql
	AgentinoLoc
	ImtCore::ImtCoreLoc
	ImtCore::imtcontrolsqml
	ImtCore::imtstylecontrolsqml
	ImtCore::imtguiqml
	ImtCore::imtguigqlqml
	ImtCore::imtauthguiqml
	ImtCore::imtcolguiqml
	ImtCore::imtdocguiqml
)



