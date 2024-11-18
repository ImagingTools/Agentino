project(agentinoqml LANGUAGES CXX)

include(${IMTCOREDIR}/Config/CMake/WebCompiler.cmake)
include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtSdlConfig.cmake)

if(QT_VERSION_MAJOR EQUAL 6)
	target_link_libraries(${PROJECT_NAME}  Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

set(buildwebdir ${BUILDDIR}/Agentino/Bin/web)

#copy html from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../Resources/html)
list(APPEND webdirs ${buildwebdir}/Resources)

# copy qml from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../Qml)
list(APPEND webdirs ${buildwebdir}/src)

# copy base qml sources from to
getImtCoreQmlWebDirs(webdirs "${buildwebdir}")

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinodata/Ddl/Qml/agentino)
list(APPEND webdirs ${buildwebdir}/src/agentino)

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoAgentsSdl)
list(APPEND webdirs ${buildwebdir}/src/agentinoAgentsSdl)

# copy Translations from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../../../Impl/AgentinoLoc/Translations)
list(APPEND webdirs ${buildwebdir}/Resources/Translations)

if(DEFINED WEB_COMPILE AND  WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
	set(resname "agentino")
	jqml_compile_web2("${webdirs}" "${buildwebdir}" "${resname}" "${sdldependency}")
endif()

