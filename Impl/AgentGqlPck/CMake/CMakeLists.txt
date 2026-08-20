project(AgentGqlPck LANGUAGES CXX)

include(${ACFDIR}/Config/CMake/ComponentConfig.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

acf_declare_target_dependencies(AgentGqlPck LINK_SCOPE PRIVATE
	agentgql
)
