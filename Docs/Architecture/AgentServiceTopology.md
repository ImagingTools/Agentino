# Agent-Managed Service Topology

## Motivation

Previously, the service topology of all agents was managed exclusively by the central
Agentino server. If the connection to the server was lost, an agent had no view of the
topology of its own services and their dependencies.

With this architecture the **agent server is the source of truth for its own services**:
each agent can resolve and provide the topology of its own services even if the central
Agentino server is not available. The central server queries this information and
visualizes the overall topology of all agents.

## Responsibilities

| Component | Responsibility |
|---|---|
| Agent server | Source of truth for its own services and their topology. Persists the service collection locally (`ServicesSettings.xml`) and the local topology layout (`TopologyCollection.xml`). Answers `GetTopology`/`SaveTopology` for its own services. |
| Agentino server | Aggregator/cache. Mirrors the service data of all agents in its `AgentCollection`, visualizes the overall topology, and marks services of offline agents. |

## Components

### Agent side

- `agentinodata::CServiceCompositeInfoBase` (`Include/agentinodata/CServiceCompositeInfoBase.h`):
  shared topology-resolution logic (URL → service, dependant connection ID → service,
  state of required services, dependency collection) operating on a single service collection.
- `agentinodata::CAgentServiceCompositeInfoComp` (`AgentinoDataPck::AgentServiceCompositeInfo`):
  agent-local implementation of `IServiceCompositeInfo` working directly on the agent's own
  `ServiceCollection` and the local service controller (`IServiceStatusProvider`).
  Dependencies on services of other agents cannot be resolved locally and are reported as
  *unknown* (not as an error).
- `agentgql::CAgentTopologyControllerComp` (`AgentGqlPck::AgentTopologyController`):
  GraphQL handler answering `GetTopology` and `SaveTopology` for the agent's own services.
  Registered in the agent's `ServicesPage` composition
  (`Partitura/AgentinoAgentVoce.arp/ServicesPage.acc`). The local topology layout is
  persisted in a local `TopologyCollection` (`Partitura/AgentinoAgentVoce.arp/AgentServer.acc`).

A client (or the central server) can query the topology of a single agent directly from the
agent, or via the central server by adding the `clientid` addition to the request, which is
forwarded to the agent by the existing request redirection mechanism.

### Server side

- `agentinogql::CTopologyControllerComp` builds the overall topology from the
  `AgentCollection` cache. It now has an optional `AgentStatusCollection` reference:
  services of agents that are currently **disconnected** are marked in the response with
  - `agentOnline = false` (new field in `Sdl/agentino/1.0/Topology.sdl`),
  - status `UNDEFINED` with the alert icon (the cached status may be outdated),
  - a warning icon for the dependency state.
- Write operations (`AddService`, `UpdateService`, `ServicesRemove`,
  `UpdateConnectionUrl`) in `agentinogql::CServiceControllerProxyComp` are forwarded to the
  agent **first** (`SendModelRequest`); the server-side `AgentCollection` cache is only
  updated after the agent confirmed the change. If the agent is offline, the mutation fails
  and the cache is left untouched.

## Offline behavior

- **Agent without central server**: the agent still answers `GetTopology`/`SaveTopology`
  for its own services from its locally persisted service collection. The status of
  services of other agents cannot be resolved and is shown as *unknown* (warning, not error).
- **Server with offline agent**: the server visualizes the last known (cached) state of the
  agent's services and marks them as offline (`agentOnline = false`).

## Follow-up work

- Push-synchronisation of topology/connection changes from the agent to the server cache via
  the existing subscription mechanism (`AgentServicesRemoteSubscriberProxy`), including a
  full reconciliation when the agent reconnects (agent state overrides the server cache).
- Initial seeding of agents that do not have local connection/topology data yet.
- Client UI: display of the `agentOnline` flag in `TopologyPage.qml`.
- An agent-local topology page in the agent web UI.
