project(agentgql LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)

target_link_libraries(${PROJECT_NAME} imtguigql imtbase imtlic imtgui imtwidgets imtauth imtservice imtclientgql agentinosdl)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

