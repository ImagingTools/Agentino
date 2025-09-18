project(AgentinoGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStdGui.cmake)

target_link_libraries(${PROJECT_NAME}
	imtbase
	imtbasesdl
	imtcom
	imtgql
	imtgui
	imtwidgets
	imtlic
	imtcol
	imtdb
	imtrest
	imtclientgql
	imtservergql
	imtserverapp
	imtguigql
	imtbase
	imtgui
	imtwidgets
	imtauth
	agentgql
	agentinogql
	agentinodata
	agentinosdl)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
