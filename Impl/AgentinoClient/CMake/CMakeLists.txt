project(AgentinoClient LANGUAGES CXX)

message(VERBOSE "ACFDIR LISA " ${ACFDIR})
message(VERBOSE "CMAKE_PROJECT_INCLUDE_BEFORE " ${CMAKE_PROJECT_INCLUDE_BEFORE})

set(ARXC_CONFIG "${PROJECT_SOURCE_DIR}/../../../Config/Agentino.awc")
set(ARXC_FILES "${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}.acc")
set(ARXC_OUTDIR "${AUX_INCLUDE_DIR}")

message(VERBOSE "ARXC_CONFIG " ${ARXC_CONFIG})

if(WIN32)
	set(CMAKE_WIN32_EXECUTABLE "on")
	set(ACF_CONVERT_FILES "${PROJECT_SOURCE_DIR}/../VC/AgentinoClient.rc.xtracf")
	set(ACF_CONVERT_REGISTRY "${PROJECT_SOURCE_DIR}/../VC/FileSubstitCopyApp.acc")
	set(ACF_CONVERT_CONFIG "${PROJECT_SOURCE_DIR}/../../../Config/BaseOnly.awc")
	set(RC_FILE "${AUX_INCLUDE_DIR}/${PROJECT_NAME}/AgentinoClient.rc")
endif()

if(APPLE)
	set(CMAKE_MACOSX_BUNDLE "on")
endif()

include(${IMTCOREDIR}/Config/CMake/WebCompiler.cmake)
include(${ACFDIR}/Config/CMake/CustomBuild.cmake)
include(${ACFDIR}/Config/CMake/ApplicationConfig.cmake)

target_link_libraries(${PROJECT_NAME} imtrest imtlicgui imtlicgql imtclientgql imtgql imtauthdb imtdb imtbase imtapp imtfile imtcom iser)
target_link_libraries(${PROJECT_NAME} i2d idoc imtqml imtstyle imtauthgui imtauthgql imtstylecontrolsqml imtgui imtguigql imtlic imtauth imtauthsdl imtappsdl imtbasesdl imtcolorsdl)
target_link_libraries(${PROJECT_NAME} ImtCoreLoc AgentinoLoc AcfLoc AcfSlnLoc)
target_link_libraries(${PROJECT_NAME} agentinodata agentinogql agentgql agentinoqml agentinosdl)
target_link_libraries(${PROJECT_NAME} imtcontrolsqml imtdocguiqml imtcolguiqml imtguiqml imtguigqlqml imtauthguiqml imtlicguiqml)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::WebSockets)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::Widgets)


include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStd.cmake)
include(${IMTCOREDIR}/Config/CMake/OpenSSL.cmake)
include(${ACFDIR}/Config/CMake/WindeployQt.cmake)

message(VERBOSE "PROJECT_BINARY_DIR ${IMTCOREDIR}/Bin/web")


if(WIN32)
	list(APPEND listOptions "--qmldir=${IMTCOREDIR}/Qml")
	set(files \"${AUX_INCLUDE_DIR}/../../../Bin/${CMAKE_BUILD_TYPE}_${TARGETNAME}/${PROJECT_NAME}.exe\")

	windeploy(${PROJECT_NAME} "${listOptions}" "${files}")
	unset(listOptions)
endif()



