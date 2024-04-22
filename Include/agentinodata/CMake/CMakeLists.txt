project(agentinodata LANGUAGES CXX)	

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${IMTCOREDIR}/Config/CMake/DdlCreator.cmake)

set(MODULE_CPP_NAME "agentino")
set(MODULE_QML_NAME "agentino")
set(DDL_CONST_VAR_TEMPLATE_INPUT_DIR "${PROJECT_SOURCE_DIR}/../Resources/Ddl")
set(DDL_OUTPUT_DIR "${AUX_INCLUDE_DIR}/${PROJECT_NAME}/Ddl")

# GenerateDdlCode(${MODULE_CPP_NAME}
# 	${MODULE_QML_NAME}
# 	${DDL_CONST_VAR_TEMPLATE_INPUT_DIR}
# 	${DDL_OUTPUT_DIR})


target_link_libraries(${PROJECT_NAME} imtservice)


