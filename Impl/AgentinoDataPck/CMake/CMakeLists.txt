project(AgentinoDataPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

acf_declare_target_dependencies(${PROJECT_NAME} LINK_SCOPE PRIVATE
	agentinodata
)

