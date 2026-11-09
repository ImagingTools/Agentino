# Tokens of the output connections

A server of a product refuses the requests of another product unless that product's server presents a token it
issued (`x-server-token`). Agentino sets such a token up on the edge of the topology that already describes the
connection: the output connection of a service linked to the input connection of a server.

## Who does what

| Part | Responsibility |
|---|---|
| Receiving server (Puma, later any) | Issues tokens for itself only, with the permissions the operator chooses (`CreateServerToken`, `UpdateServerTokenScopes`, `RevokeServerToken`). Agentino never decides permissions. |
| Agentino server | Orchestrates: finds the link and the address of the receiving server in its mirror, opens a superuser session there with the superuser password the operator types (the login of the superuser is fixed; the password is not kept), makes the product of the service its client (Puma adds it to its catalog, a receiver with clients in its configuration only confirms them), asks it for the permission tree and for the token, hands the token to the agent. |
| Access | Every command of the connection tokens needs a superuser session of Agentino; anonymous and ordinary users are refused. The superuser session at the receiving server stays on the Agentino server: the browser gets a one-time sign-in ID bound to its operator and connection. The session ends right after the token is issued, changed or revoked, when the operator cancels, and 10 minutes after the sign-in at the latest. Tests: `Tests/ConnectionTokenApiPostman`. |
| Receivers | The settings plugin of a service declares the inputs through which it receives server tokens (`ServerTokenConnectionIds`, Puma and Lisa). The agent reports it with every `GetService` (`InputConnection.acceptsServerTokens`), the server keeps it in its mirror, not on disk. `GetConnectionToken` answers `canIssue` and the reason when a token cannot be issued: the connection goes to an input that receives no tokens, or the agent of that server is not connected. Revoking a held token does not need it. |
| Agent | Keeps the token per service and output connection in `ConnectionTokens.json`, the value protected by the system (DPAPI on Windows, a file readable by the account of the agent elsewhere), and passes it to the service at its start in `IMT_<RECEIVER>_SERVER_CREDENTIAL`. The value never leaves the agent. |
| Service | Reads the variable as before (`CServerCredentialProviderComp`); it does not know Agentino. |

## Components

- ImtCore `imtauthgql::IServerTokenIssuer` / `CRemoteServerTokenIssuerComp` — the token API of a remote server over a GraphQL client whose address is set per request.
- ImtCore `imtcrypt::ISecretProtector` / `CSystemSecretProtectorComp` — protection of a secret at rest.
- ImtCore `imtclientgql::GetServerCredentialVariableName()` — the one rule naming the variable.
- `agentinogql::CConnectionTokenControllerComp` — `ConnectionTokens.sdl`, between the UI and the Agentino server.
- `agentgql::CAgentConnectionTokenControllerComp` — `AgentConnectionTokens.sdl`, between the Agentino server and the agent.
- `agentinodata::IConnectionTokenStore` and `IServiceEnvironmentProvider`, implemented by `CConnectionTokenStoreComp`; `CServiceSupervisorComp` adds the environment of a service to its start (`IProcessHost::SpawnRequest::environment`).
- QML `ConnectionTokenView` (view) and `ConnectionTokenTransport` (L3), one transport per output connection; shown in the service editor of the Agentino server only.

## Flow

1. The operator opens the output connection of a linked service and chooses *Issue Token...*.
2. The password of the superuser of the receiving server; the Agentino server answers with the permission tree and a session.
3. The operator checks permissions; the receiving server issues the token, the agent stores it.
4. The service gets the token at its next start. A change of permissions keeps the token and needs no restart.
5. *Revoke* revokes the token at the receiving server and removes it from the agent.

The console of the receiving server (Puma's product page) stays an alternative for installations without Agentino.
