project(AgentinoGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStdGui.cmake)

target_link_libraries(${PROJECT_NAME}
	imtbase
	imtgql
	imtgui
	imtwidgets
	imtlic
	imtdb
	imtrest
	imtguigql
	imtbase
	imtgui
	imtwidgets
	imtauth
	agentinogql
	agentinodata
	agentinosdl
	agentgql)
