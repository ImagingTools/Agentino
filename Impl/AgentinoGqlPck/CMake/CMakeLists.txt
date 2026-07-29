project(AgentinoGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStdGui.cmake)

target_link_libraries(${PROJECT_NAME}
	imtbase
	imtbasesdl
	imtsdl
	imtcol
	imtcom
	imtgql
	imtgui
	imtwidgets
	imtlic
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
	agentinodata
	agentinosdl
	agentgql
	agentinogql
)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
