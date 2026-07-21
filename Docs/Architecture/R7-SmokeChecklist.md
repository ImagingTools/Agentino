# R7 — End-to-end smoke checklist

Run after rebuild of **imtclientgql** + Agentino packages/apps.

## Environment

- [ ] AgentinoServer (or ClientServer) up
- [ ] AgentinoAgent connected and **Approved**
- [ ] At least one AutoStart-capable service registered on the agent

## Scenarios

| # | Scenario | Expected | Pass |
|---|----------|----------|------|
| 1 | Fresh service Start | `Starting` → `Running` (async Start returns STARTING immediately; live push → Running) | |
| 2 | Kill service process (AutoStart) | Crashed → backoff → respawn → Running | |
| 3 | Crash-loop (start-then-die) | After `maxRestarts` in window → Failed/CrashLooping; budget not cleared on bare start | |
| 4 | `kill -9` agent, restart agent | Services **adopted** Running, **no** second process; death of adopted → normal restart | |
| 5 | Enroll new agent | Pending, no domain ingest; Approve → resync; **Revoke while connected** stops status/collection ingest | |
| 6 | ServicesList for connected agent | Rows from **Fleet** (not empty when services exist); offline agent empty fleet or last projection | |
| 7 | Cross-agent bind (if available) | Catalog pick-list; offline producer → EndpointOffline | |
| 8 | Reconnect after gap | Reconcile via async ServicesList; fleet snapshot updated | |
| 9 | QML TypedBackend mock | Approvals/topology views work without GQL backend | |

## Quick commands (Windows examples)

```text
# After agent is running with a supervised service PID in ServiceRuntimePids.json:
taskkill /F /PID <agent_pid>
# restart AgentinoAgent.exe — service should adopt, not duplicate

# Kill only the child service:
taskkill /F /PID <service_pid>
# expect AutoStart respawn
```

## Notes

- Optimistic Start/Stop: GUI may show STARTING/STOPPING before agent ack; final state from subscription.
- Fleet-owned agents: ServicesList does not fall back to ServiceManager mirror when a fleet session exists.
