project(agentinogql LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCoreProjectSdlConfig.cmake)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

