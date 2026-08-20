project(AgentinoDataPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

agentino_declare_library_dependencies(${PROJECT_NAME} LINK_SCOPE PRIVATE
	agentinodata
)

