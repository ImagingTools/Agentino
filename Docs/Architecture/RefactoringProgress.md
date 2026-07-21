# Agentino Architecture Refactoring — Final status

Source plan: [AgentinoArchitectureAudit.md](./AgentinoArchitectureAudit.md)  
Deploy: [Deploy.md](./Deploy.md)

## Implemented end-to-end in Agentino

| Plan item | Implementation |
|-----------|----------------|
| Step 0 foreign scaffolding | Orphan `Server.acc` removed |
| Step 1 protocol | `agentproto` envelopes, outbox, gateway, Hello/Ping/Resync |
| Typed publish | Outbox → `typedEventReady` JSON frames; **drain uses absolute sequence** (survives ring trim) |
| Step 2 one link | Pure outbound agent WS client; no su/1/1111 |
| Step 2b enrollment | Store/gate/challenge/GQL admin/Approvals page |
| Step 3 supervisor | ProcessHost + FSM; ServiceController cutover |
| Step 4 fleet | Snapshot + live status; **ListObjects prefers Fleet**; **SetupGqlItem fills from Fleet when mirror miss**; full descriptor fields in snapshot |
| Step 5 type catalog | Load once per type on Start |
| Step 5b connections | Catalog + BindingOrchestrator + propagate |
| Step 6 shells | ClientServer pure client |
| Step 7 topology layout | Client/view-only |
| QML QG0–QG5 | Ports, Topology/ServiceEditor transport, TypedBackend mock (factory parity with AgentinoBackend) |
| P14 enum | RUNNING_IMPOSSIBLE serialized |
| Agent protocol GQL | `ProtocolRevisions` / `ProtocolResync` / `ProtocolRouteCommand` / `ProtocolDrainEvents` wired in agent **Handlers** (thin ExportId → AgentGateway/EventOutbox) |
| Dual-write status | Fleet primary; ServiceStatusCollection skips identical writes |

## Operator surfaces

- Pages: **Topology**, **Agents**, **Approvals**
- GQL: enrollment admin + `FleetServicesList` / `AgentSessionStatus` / `FleetAgentsSessions`
- Agent GQL (local): `ProtocolRevisions` / `ProtocolResync` / `ProtocolRouteCommand` / `ProtocolDrainEvents`

## ACC notes (Agentino residual)

- **R1.4:** `AgentCollection` is records-only; **`AgentServiceManager`** owns `IServiceManager` (`Repositories/AgentServiceManager`, export `ServiceManager`). ObjectCollection for Services still points at AgentRepository until Fleet facade (R1.1).
- **FleetReadModel** exported from `ServerBase.acc`; Handlers/Pages use ExportId thin wiring.
- **Agent protocol GQL** lives under agent `Handlers.acc` (same pattern as server Fleet/Enrollment GQL in Handlers).
- Completion plan: [RefactoringCompletionPlan.md](./RefactoringCompletionPlan.md)

## Review fixes (post-audit)

| Finding | Fix |
|---------|-----|
| **HIGH** ProcessHost `ChildStarted` never fired | Insert child into `m_children` **before** `start()`/`waitForStarted` |
| **HIGH** crash auto-restart stuck in Starting | Stay **Crashed** during backoff; `OnRestartBackoff` → `Start()` (Crashed+Start→Starting+spawn). `Start()` recovers stuck Starting/Running without live child |
| **MEDIUM** enrollment only on connect | Live status/collection pushes + mirror reconcile gate on `EnrollmentController::Get == Approved`; drop subs when denied |
| **MEDIUM** restart budget / crash-loop | Window via `restartWindowStart` + `windowSeconds`; clear count only after **stable Running** (`StableSeconds` health timer, default 15s) — not on bare `ChildStarted` |
| **LOW** dual Starting emit | Removed extra `EmitStatus` after Spawn |
| **LOW** Starting→Stopping | FSM accepts `Stop` from `Starting` |
| **LOW** Stop when no child | Mark stopped only if host reports not running |
| **Adoption after agent restart (#4)** | Durable `ServiceRuntimePids.json` (pid + program); `ProcessHost::TryAdopt` by explicit PID + image-path verify; poll liveness for adopted; AutoStart/Start adopt before spawn. **No** process-table scan / image-name kill |

## Completion plan (code complete pending R7 operator run)

See [RefactoringCompletionPlan.md](./RefactoringCompletionPlan.md) and [R7-SmokeChecklist.md](./R7-SmokeChecklist.md).

| Item | Status |
|------|--------|
| R5 OD1–OD4 DECIDED | **Done** |
| R1.4 ServiceManager role split | **Done** |
| R1.3 nested services out of `CAgentInfo` | **Done** |
| R1.2 dual-write / RemoveStale | **Done** (Fleet ApplyEvent on sync/remove) |
| R1.1 Fleet-only list when agent in fleet | **Done** |
| R2 async WS + reconcile + optimistic Start/Stop | **Done** (when AsyncApiClient wired) |
| R6 ACC role hygiene | **Done** |
| R7 live smoke | **Checklist ready — run on real system** |
| R3 typed-WS physical wire | Deferred (optional) |

## R2 (ImtCore + Agentino) — nested loops

| Piece | Status |
|-------|--------|
| `IAsyncGqlClient` on HTTP `CAsyncApiClientComp` | Pre-existing |
| `CSubscriptionManagerComp` = **async only** (`IAsyncGqlClient`); sync via `CGqlClientSyncAdapterComp` | **Split** |
| `WebSocketServerFramework`: `IAsyncGqlClient`→SubscriptionManager, `IGqlClient`→GqlClientSyncAdapter | **Done** |
| `TClientRequestManagerCompWrap` = sync only; `TAsyncClientRequestManagerCompWrap` = async only | **Split** |
| Agentino `CServiceControllerProxyComp` stacks both wraps; reconcile/Start/Stop async-only | **Done** |
| Full GQL mutation handlers without Wait | Open (needs async GQL response framework) |
| **R3** typed-WS physical wire | Deferred |

## Outside Agentino-only residual

1. **R2 remainder** — migrate Start/Stop/GetService call sites fully off nested Wait  
2. **R3** — Physical typed WS wire (optional polish)

## Smoke to run after rebuild

1. Start service → must reach **Running** (not stuck in Starting)  
2. **AutoStart crash recovery:** kill supervised process externally → status Crashed → after backoff respawns → Running  
3. **Crash loop:** service that exits immediately after start, `maxRestarts` times in window → **Failed/CrashLooping** (budget must NOT reset on bare ChildStarted)  
4. Stable Running for `StableSeconds` (default 15s) → `restartCount` cleared  
5. Revoke connected agent → further status/collection pushes ignored; fleet evicted  
6. **Adoption:** start AutoStart service → hard-kill agent (not the service) → restart agent → service stays single instance (adopted, Running); no second process  

## Build

Full rebuild of agentproto, agentinodata, agentgql, agentinogql, agentinoqml, packages, apps.
