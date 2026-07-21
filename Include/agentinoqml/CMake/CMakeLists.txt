project(agentinoqml LANGUAGES CXX)

include(${IMTCOREDIR}/Config/CMake/WebCompiler.cmake)
include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtSdlConfig.cmake)

if(QT_VERSION_MAJOR EQUAL 6)
	target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Core5Compat)
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

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoServicesSdl)
list(APPEND webdirs ${buildwebdir}/src/agentinoServicesSdl)

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoTopologySdl)
list(APPEND webdirs ${buildwebdir}/src/agentinoTopologySdl)

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoEnrollmentSdl)
list(APPEND webdirs ${buildwebdir}/src/agentinoEnrollmentSdl)

# copy Translations from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../../../Impl/AgentinoLoc/Translations)
list(APPEND webdirs ${buildwebdir}/Resources/Translations)

set(QML_IMPORT_DIRS ${QML_IMPORT_PATH})
list(APPEND QML_IMPORT_DIRS "${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoServicesSdl")
list(REMOVE_DUPLICATES QML_IMPORT_DIRS)

set(QML_IMPORT_PATH ${QML_IMPORT_DIRS}
	CACHE STRING "Qt Creator extra qml import paths for ${PROJECT_NAME}"
	FORCE
)

option(USE_NEW_WEB "Use new web compilation (jq_compile_web)" OFF)

if(DEFINED WEB_COMPILE AND WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
	set(resname "agentino")
	if(USE_NEW_WEB)
		set(inputjs "${PROJECT_SOURCE_DIR}/../agentino.json")
		set(appicon "../Icons/AgentinoIcon.svg")
		set(dataroot "/Agentino/Views/")
		set(startqml "${PROJECT_SOURCE_DIR}/../Qml/AgentinoWeb.qml")

		jq_compile_web(
			"${webdirs}"
			"${buildwebdir}"
			"${resname}"
			"${inputjs}"
			"${appicon}"
			"${dataroot}"
			"${startqml}"
		)
	else()
		jqml_compile_web2(
			"${webdirs}"
			"${buildwebdir}"
			"${resname}"
			"${sdldependency}"
		)
	endif()
endif()

