# Agentino deployment (post-architecture redesign)

## Processes

| Process | Role | Notes |
|---------|------|--------|
| **AgentinoServer** | Headless central server | GraphQL `Agentino/graphql`, enrollment, fleet projections |
| **AgentinoAgent** | Host agent | Service supervisor, pure outbound WS client + local WSS for GUI |
| **AgentinoClient** | Pure GUI client | Talks GraphQL to server (or agent for single-host) |
| **AgentinoClientServer** | Pure GUI (server no longer embedded) | Same as Client; run **AgentinoServer** separately |

## Ports (defaults)

| Side | HTTP | WebSocket |
|------|------|-----------|
| Server | 7111 | 7112 |
| Agent (local) | 7222 | 7223 |

## Enrollment

1. Agent generates fingerprint under AppData `agent_identity.key`.
2. On connect: `GetEnrollmentChallenge` → `AgentAdd` with `challengeResponse`.
3. First agent may be bootstrap-approved; later agents **Pending** until **Approvals** page (`su` can approve).
4. `RequireChallengeResponse` defaults to **true**.

## Operator UI

- **Topology**, **Agents**, **Approvals** pages on the server GUI.
- Approvals: Pending list, Approve / Reject.
