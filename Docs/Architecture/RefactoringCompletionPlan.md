# Agentino Refactoring — Completion Plan (closing the plan to 100%)

> Companion to [AgentinoArchitectureAudit.md](./AgentinoArchitectureAudit.md) and
> [RefactoringProgress.md](./RefactoringProgress.md).

## Progress (execution log) — current

| Item | Status | Notes |
|------|--------|-------|
| **R5** OD1–OD4 | **DONE** | §7.4 DECIDED |
| **R1.4** triple-role split | **DONE** | AgentCollection records-only; AgentServiceManager = IServiceManager |
| **R1.3** nested `CAgentInfo` services | **DONE** | Nested collection removed; mirrors in AgentServiceManager |
| **R1.2** dual-write | **DONE** | Fleet ApplyEvent on sync/remove; ServiceManager = connection-meta projection; RemoveStale path deleted |
| **R1.1** Fleet-only reads | **DONE** | ListObjects: if fleet has session/projections → **only** Fleet (incl. empty); ServiceManager only pre-session bootstrap |
| **R2** nested loops | **DONE for hot paths** | Async WS client; async reconcile; **async optimistic Start/Stop** (no Wait on GQL worker when AsyncApiClient set). Other mutations may still Wait. |
| **R3** typed-WS | **deferred** | Optional polish, not conformance |
| **R6** ACC hygiene | **DONE** | Distinct roles/exports |
| **R7** live smoke | **checklist ready** | [R7-SmokeChecklist.md](./R7-SmokeChecklist.md) — must be run by operator |

## Definition of done

- [x] Nested services not inside `CAgentInfo`
- [x] No AgentCollection multi-role as ServiceManager
- [x] `RemoveStaleMirrorServicesByPath` gone
- [x] OD1–OD4 DECIDED
- [x] Async server→agent client path
- [x] Fleet is service **list** SoT when agent is in fleet
- [x] Start/Stop GQL path without nested Wait (async + optimistic)
- [ ] R7 scenarios green **on a running system** (operator)

## What remains outside code completion

1. **R7 operator run** of the smoke matrix  
2. **R3** physical typed-WS frame channel — only if measured  

ServiceManager remains a **projection store** for connection topology meta (not nested in agent records). Full drop of ServiceManager requires topology/connection consumers to read only Fleet descriptors — optional follow-up.

## Sequencing (historical)

```
R5 → R1.4 → R1.3 → R1.2 → R1.1 → R2 async → R7 smoke
R3 deferred
```
