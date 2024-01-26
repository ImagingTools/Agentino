project(agentinoqml LANGUAGES CXX)

include(${IMTCOREDIR}/Config/CMake/WebCompiler.cmake)
include(${ACFDIR}/Config/CMake/StaticConfig.cmake)

if(QT_VERSION_MAJOR EQUAL 6)
	find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Core5Compat REQUIRED)
	target_link_libraries(${PROJECT_NAME}  Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

set(buildwebdir ${CMAKE_CURRENT_LIST_DIR}/../../../Bin/web)

list(APPEND webdirs ${IMTCOREDIR}/3rdParty/JQML2/Core/dist)
list(APPEND webdirs ${buildwebdir}/Resources)

#copy html from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../Resources/html)
list(APPEND webdirs ${buildwebdir}/Resources)

# copy qml from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../Qml)
list(APPEND webdirs ${buildwebdir}/src)

# copy base qml sources from to
list(APPEND webdirs ${IMTCOREDIR}/Include/imtstylecontrolsqml/Qml/Fonts)
list(APPEND webdirs ${buildwebdir}/Resources)

list(APPEND webdirs ${IMTCOREDIR}/Include/imtstylecontrolsqml/Qml/Acf)
list(APPEND webdirs ${buildwebdir}/src/Acf)

list(APPEND webdirs ${IMTCOREDIR}/Qml/web)
list(APPEND webdirs ${buildwebdir}/Resources)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtgui)
list(APPEND webdirs ${buildwebdir}/src/imtgui)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtguigql)
list(APPEND webdirs ${buildwebdir}/src/imtguigql)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtauthgui)
list(APPEND webdirs ${buildwebdir}/src/imtauthgui)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtlicgui)
list(APPEND webdirs ${buildwebdir}/src/imtlicgui)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtcolgui)
list(APPEND webdirs ${buildwebdir}/src/imtcolgui)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtdocgui)
list(APPEND webdirs ${buildwebdir}/src/imtdocgui)

list(APPEND webdirs ${IMTCOREDIR}/Qml/imtcontrols)
list(APPEND webdirs ${buildwebdir}/src/imtcontrols)


# message("SMDIR ${SMDIR}")

if(DEFINED WEB_COMPILE AND  WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
	set(resname "agentino")
	jqml_compile_web2("${webdirs}" "${buildwebdir}" "${resname}")
endif()

