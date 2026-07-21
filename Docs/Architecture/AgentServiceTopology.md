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

- `agentinodata::CServiceCompositeInfoBase` — shared topology-resolution logic.
- `agentinodata::CAgentServiceCompositeInfoComp` — agent-local `IServiceCompositeInfo`.
- `agentgql::CAgentTopologyControllerComp` — `GetTopology`/`SaveTopology` for own services; sets `agentOnline=true`.
- Dual notifiers on `ServiceCollection`:
  - `LocalServiceCollectionSubscriberController` → `OnServicesCollectionChanged` (agent GUI, WebSocket server).
  - `RemoteServiceCollectionSubscriberController` → `OnAgentServicesCollectionChanged` (central server, WebSocket client).

### Server side

- `agentinogql::CTopologyControllerComp` — overall topology from mirror; `agentOnline` + offline icons.
- `agentinogql::CServiceControllerProxyComp` — mutations forward to agent first; implements `IServiceCollectionSynchronizer`.
- `agentinogql::CSubscriptionControllerComp` — per-agent status + collection subscriptions; reconciles mirror on push.
- `agentinogql::CAgentCollectionControllerComp` — on every AgentAdd/(re)connect (500ms timer): **full** `SyncAgentServicesInMirror` then status refresh.

## Synchronisation channels

| Channel | Direction | When |
|---|---|---|
| Proxy GQL mutations | Server GUI → Agent → Mirror | Add/Update/Remove/Start/Stop… |
| Push `OnAgentServicesCollectionChanged` | Agent → Server mirror | Online insert/update/remove on agent |
| Full reconcile `SyncAgentServicesInMirror` | Server pulls agent `ServicesList` + `GetService` | Every (re)connect |
| Status `OnAgentServiceStatusChanged` | Agent → Server status collection | Live status |
| GUI `OnServicesCollectionChanged` / `OnAgentStatusChanged` | Server → Topology UI | Mirror/status/online changes |

## Offline behavior

- **Agent without central server**: answers local `GetTopology`/`SaveTopology`; deps on other agents = warning.
- **Server with offline agent**: last known mirror; `agentOnline=false`, status `UNDEFINED`, Alert/Warning icons. Services remain until purged.

## Topology UI (`TopologyPage.qml`)

- Shared by server and agent apps.
- **New service**: server → agent picker (`Agents` document service); agent → create without picker/`clientid`.
- Live refresh: `OnServicesCollectionChanged`, `OnTopologyChanged`, `OnAgentStatusChanged`, `OnServiceStatusChanged` (respects `m_agentOnline`).

## Layout coordinates

`TopologyCollection` x/y are **local to each side** (server app vs agent app).  
They are **not** part of agent↔server service synchronisation and must not be synced.

## Detailed implementation plan

See `Docs/Architecture/ServiceSyncArchitecturePlan.md`.
