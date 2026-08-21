project(AgentinoDataPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

target_link_libraries(${PROJECT_NAME} PRIVATE
	agentinodata
)

