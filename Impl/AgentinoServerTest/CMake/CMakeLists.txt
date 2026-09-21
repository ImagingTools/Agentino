project(AgentinoServerTest LANGUAGES CXX)

set(ARXC_CONFIG "${PROJECT_SOURCE_DIR}/../../../Config/Agentino.awc")
set(ARXC_FILES "${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}.acc")
set(ARXC_OUTDIR "${AUX_INCLUDE_DIR}")

include(${ACFDIR}/Config/CMake/CustomBuild.cmake)
include(${ACFDIR}/Config/CMake/ApplicationConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/OpenSSL.cmake)
include(${IMTCOREDIR}/Config/CMake/Quazip.cmake)
include(${ACFDIR}/Config/CMake/DeployQt.cmake)

if(DEFINED WEB_COMPILE AND WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
endif()

if(DEFINED USE_NEW_WEB AND USE_NEW_WEB)
	add_compile_definitions(AGENTINO_USE_NEW_WEB)
endif()

set(DEPLOY_QT_QML_DIRS "${IMTCOREDIR}/Qml")
deploy_qt(TARGET ${PROJECT_NAME})
unset(DEPLOY_QT_QML_DIRS)

if(APPLE)
	file(COPY ${PROJECT_SOURCE_DIR}/../../../Build/Migrations DESTINATION ${AGENTINODIR_BUILD}/Bin/${CMAKE_BUILD_TYPE}_${TARGETNAME}/${PROJECT_NAME}.app/Contents/MacOS)
else()
	file(COPY ${PROJECT_SOURCE_DIR}/../../../Build/Migrations DESTINATION ${AGENTINODIR_BUILD}/Bin/${CMAKE_BUILD_TYPE}_${TARGETNAME})
	file(COPY ${PROJECT_SOURCE_DIR}/../../../Build/LogMigrations DESTINATION ${AGENTINODIR_BUILD}/Bin/${CMAKE_BUILD_TYPE}_${TARGETNAME})
endif()

include(${IACFDIR}/Config/CMake/zlib.cmake)

add_definitions(-DUNICODE -D_UNICODE)

target_link_libraries(${PROJECT_NAME} PRIVATE
	agentinoqml
	agentinogql
	agentgql
	AgentinoLoc
	ImtCore::ImtCoreLoc
	AcfSln::iservice
	ImtCore::imtauthgql
	ImtCore::imtauthdb
	ImtCore::imtchatdb
	ImtCore::imtdeskdb
	ImtCore::imtlog
	ImtCore::imtstylecontrolsqml
	ImtCore::imtlicgui
	ImtCore::imtstyle
	ImtCore::imtgui
	ImtCore::imtguigql
	ImtCore::imtauthgui
)
