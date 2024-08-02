project(agentinogql LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCoreProjectSdlConfig.cmake)

ImtCoreConfigureSdlCpp(
	"APIv1_0"
	"CPP"
	"${PROJECT_NAME}::sdl"
	"-MVMap;-MTreeModel;-MGraphQL;--GQL;--CPP"
)

target_link_libraries(${PROJECT_NAME} imtguigql imtbase imtlic imtgui imtwidgets imtauth agentinodata)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

