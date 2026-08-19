project(AgentGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

agentino_declare_library_dependencies(AgentGqlPck LINK_SCOPE PRIVATE
	agentgql
)
