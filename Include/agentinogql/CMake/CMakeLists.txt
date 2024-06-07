project(agentinogql LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/StaticConfig.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCore.cmake)
include(${IMTCOREDIR}/Config/CMake/ImtCoreSdl.cmake)

ImtCoreGenerateSdl(
	"${CMAKE_CURRENT_LIST_DIR}/../SDL/APIv1_0.sdl"
	"${AUX_INCLUDE_DIR}/${PROJECT_NAME}/SDL/CPP"
	"${PROJECT_NAME}::sdl"
	"-MVMap;-MTreeModel;-MGraphQL;--GQL;--CPP"
)

ImtCoreGenerateSdl(
	"${CMAKE_CURRENT_LIST_DIR}/../SDL/APIv1_0.sdl"
	"${AUX_INCLUDE_DIR}/${PROJECT_NAME}/SDL/QML/${PROJECT_NAME}Sdl"
	"${PROJECT_NAME}Sdl"
	"--QML"
)

ImtCoreFutureResourceWrap (
	${PROJECT_NAME}
	"${AUX_INCLUDE_DIR}/${PROJECT_NAME}/SDL/QML/${PROJECT_NAME}Sdl/${PROJECT_NAME}Sdl.qrc"
)

target_link_libraries(${PROJECT_NAME} imtguigql imtbase imtlic imtgui imtwidgets imtauth agentinodata)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

