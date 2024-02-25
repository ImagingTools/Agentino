# Root of Agentino project
TEMPLATE = subdirs

# Libraries
SUBDIRS += agentgql
agentgql.file = ../../Include/agentgql/QMake/agentgql.pro

SUBDIRS += agentinodata
agentinodata.file = ../../Include/agentinodata/QMake/agentinodata.pro

SUBDIRS += agentinogql
agentinogql.file = ../../Include/agentinogql/QMake/agentinogql.pro

SUBDIRS += agentinoqml
agentinoqml.file = ../../Include/agentinoqml/QMake/agentinoqml.pro

SUBDIRS += AgentinoDataPck
AgentinoDataPck.file = ../../Impl/AgentinoDataPck/QMake/AgentinoDataPck.pro
AgentinoServer.depends = agentinodata

SUBDIRS += AgentinoGqlPck
AgentinoGqlPck.file = ../../Impl/AgentinoGqlPck/QMake/AgentinoGqlPck.pro
AgentinoGqlPck.depends = agentinogql

# Application

SUBDIRS += AgentinoServer
AgentinoServer.file = ../../Impl/AgentinoServer/QMake/AgentinoServer.pro
AgentinoServer.depends = agentinogql agentinoqml

SUBDIRS += AgentinoAgent
AgentinoAgent.file = ../../Impl/AgentinoAgent/QMake/AgentinoAgent.pro
AgentinoAgent.depends = agentinogql agentinoqml

SUBDIRS += AgentinoClient
AgentinoClient.file = ../../Impl/AgentinoClient/QMake/AgentinoClient.pro
AgentinoClient.depends = agentinogql agentinoqml


