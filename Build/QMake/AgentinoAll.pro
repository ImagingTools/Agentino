# Root of Agentino project
TEMPLATE = subdirs

# Libraries
SUBDIRS += agentgql
agentgql.file = ../../Include/agentgql/QMake/agentgql.pro
agentgql.depends = agentinosdl

SUBDIRS += agentinodata
agentinodata.file = ../../Include/agentinodata/QMake/agentinodata.pro
agentinodata.depends = agentinosdl

SUBDIRS += agentinogql
agentinogql.file = ../../Include/agentinogql/QMake/agentinogql.pro
agentinogql.depends = agentinosdl

SUBDIRS += agentinoqml
agentinoqml.file = ../../Include/agentinoqml/QMake/agentinoqml.pro
agentinoqml.depends = agentinosdl

SUBDIRS += agentinosdl
agentinosdl.file = ../../Sdl/agentino/QMake/agentinosdl.pro

SUBDIRS += AgentinoDataPck
AgentinoDataPck.file = ../../Impl/AgentinoDataPck/QMake/AgentinoDataPck.pro
AgentinoServer.depends = agentinodata

SUBDIRS += AgentinoGqlPck
AgentinoGqlPck.file = ../../Impl/AgentinoGqlPck/QMake/AgentinoGqlPck.pro
AgentinoGqlPck.depends = agentinogql agentinosdl

SUBDIRS += AgentGqlPck
AgentGqlPck.file = ../../Impl/AgentGqlPck/QMake/AgentGqlPck.pro
AgentGqlPck.depends = agentgql

SUBDIRS += AgentinoLoc
AgentinoLoc.file = ../../Impl/AgentinoLoc/QMake/AgentinoLoc.pro

# Application

SUBDIRS += AgentinoServer
AgentinoServer.file = ../../Impl/AgentinoServer/QMake/AgentinoServer.pro
AgentinoServer.depends = agentinogql agentinoqml agentinodata

SUBDIRS += AgentinoAgent
AgentinoAgent.file = ../../Impl/AgentinoAgent/QMake/AgentinoAgent.pro
AgentinoAgent.depends = agentinogql agentinoqml

SUBDIRS += AgentinoClient
AgentinoClient.file = ../../Impl/AgentinoClient/QMake/AgentinoClient.pro
AgentinoClient.depends = agentinogql agentinoqml


