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

## Topology groups (composite nodes, server only)

`GetTopology` returns a tree of nodes (`Service` with `nodeType` / `isComposite` / `parentId`):

| `nodeType` | Composite | Default `parentId` |
|---|---|---|
| `AGENT` | yes | root (empty) |
| `SERVICE` | no | its agent |
| `GROUP` (user-defined) | yes | root (empty) |

- A user group is a `TopologyCollection` element of type `TopologyGroup`
  (`agentinodata::CTopologyGroup`: position + IDs of its direct members, name = element name).
  Membership in a group overrides the default parent; a node belongs to one group only.
- Mutations of `CTopologyControllerComp`: `CreateTopologyGroup`, `MoveTopologyNodes`
  (empty target = back to the default parent), `RemoveTopologyGroup` (members move one level up).
  Cycles are rejected. Positions of agents/groups are saved by `SaveTopology` like services.
- `CAgentTopologyControllerComp` does not support groups (the agent GUI shows its services flat).
- UI: each page shows the children of `TopologyPage.groupId` (empty = root). Opening a composite
  node (double click / Enter / Edit) adds a closable tab with another `TopologyPage`
  (`MultiDocWorkspaceView.addFixedView`); "Service" documents stay registered by the root page.

## Layout coordinates

`TopologyCollection` x/y are **local to each side** (server app vs agent app).  
They are **not** part of agent↔server service synchronisation and must not be synced.

## Detailed implementation plan

See `Docs/Architecture/ServiceSyncArchitecturePlan.md`.
