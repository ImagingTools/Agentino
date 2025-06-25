project(AgentGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStdGui.cmake)

target_link_libraries(${PROJECT_NAME}
	imtbase
	imtbasesdl
	imtcol
	imtcom
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
	imtservergql
	agentinodata
	agentinosdl
	agentgql
)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
