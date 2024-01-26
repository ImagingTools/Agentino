project(agentgql LANGUAGES CXX)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebSockets)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)

target_link_libraries(${PROJECT_NAME} imtguigql imtbase imtlic imtgui imtwidgets imtauth)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

