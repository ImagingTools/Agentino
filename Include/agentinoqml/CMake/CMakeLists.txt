project(agentinoqml LANGUAGES CXX)

include(${IMTCOREDIR}/Config/CMake/WebCompiler.cmake)
include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtSdlConfig.cmake)

if(QT_VERSION_MAJOR EQUAL 6)
	target_link_libraries(${PROJECT_NAME}  Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

set(buildwebdir ${BUILDDIR}/Agentino/Bin/web)

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

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinodata/Ddl/Qml/agentino)
list(APPEND webdirs ${buildwebdir}/src/agentino)

list(APPEND webdirs ${AUX_INCLUDE_DIR}/agentinosdl/SDL/1.0/QML/agentinoAgentsSdl)
list(APPEND webdirs ${buildwebdir}/src/agentinoAgentsSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtauthsdl/SDL/1.0/QML/imtauthGroupsSdl)
list(APPEND webdirs ${buildwebdir}/src/imtauthGroupsSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtauthsdl/SDL/1.0/QML/imtauthRolesSdl)
list(APPEND webdirs ${buildwebdir}/src/imtauthRolesSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtauthsdl/SDL/1.0/QML/imtauthUsersSdl)
list(APPEND webdirs ${buildwebdir}/src/imtauthUsersSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtauthsdl/SDL/1.0/QML/imtauthAuthorizationSdl)
list(APPEND webdirs ${buildwebdir}/src/imtauthAuthorizationSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtlicsdl/SDL/1.0/QML/imtlicFeaturesSdl)
list(APPEND webdirs ${buildwebdir}/src/imtlicFeaturesSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtlicsdl/SDL/1.0/QML/imtlicProductsSdl)
list(APPEND webdirs ${buildwebdir}/src/imtlicProductsSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtlicsdl/SDL/1.0/QML/imtlicLicensesSdl)
list(APPEND webdirs ${buildwebdir}/src/imtlicLicensesSdl)

list(APPEND webdirs ${IMTCOREDIR}/AuxInclude/${TARGETNAME}/GeneratedFiles/imtappsdl/SDL/1.0/QML/imtappApplicationSdl)
list(APPEND webdirs ${buildwebdir}/src/imtappApplicationSdl)

# copy Translations from to
list(APPEND webdirs ${PROJECT_SOURCE_DIR}/../../../Impl/AgentinoLoc/Translations)
list(APPEND webdirs ${buildwebdir}/Resources/Translations)

list(APPEND webdirs ${IMTCOREDIR}/Impl/ImtCoreLoc/Translations)
list(APPEND webdirs ${buildwebdir}/Resources/Translations)

if(DEFINED WEB_COMPILE AND  WEB_COMPILE)
	add_compile_definitions(WEB_COMPILE)
	set(resname "agentino")
	jqml_compile_web2("${webdirs}" "${buildwebdir}" "${resname}" "${sdldependency}")
endif()

