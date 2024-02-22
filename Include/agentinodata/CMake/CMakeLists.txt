project(agentinodata LANGUAGES CXX)	

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)

target_link_libraries(${PROJECT_NAME} imtservice)

#acf_use_qt_graphics_modules()

