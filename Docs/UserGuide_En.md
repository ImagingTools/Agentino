# Agentino — User Guide

## Contents

1. [What is Agentino](#1-what-is-agentino)
2. [What tasks Agentino solves](#2-what-tasks-agentino-solves)
3. [Key concepts](#3-key-concepts)
4. [System components and access methods](#4-system-components-and-access-methods)
5. [First launch: quick start](#5-first-launch-quick-start)
6. [Signing in and access rights](#6-signing-in-and-access-rights)
7. [Agentino server interface](#7-agentino-server-interface)
8. [Working with agents](#8-working-with-agents)
9. [Working with services](#9-working-with-services)
10. [Topology](#10-topology)
11. [Logs](#11-logs)
12. [Remote terminal](#12-remote-terminal)
13. [Agent local interface](#13-agent-local-interface)
14. [Typical scenarios](#14-typical-scenarios)
15. [Troubleshooting](#15-troubleshooting)
16. [Status reference](#16-status-reference)

---

## 1. What is Agentino

**Agentino** is a system for centralized management of services (programs,
applications, server processes) running on many computers across a network.

A small program — the **agent** (AgentinoAgent) — is installed on each
managed computer. The agent starts, stops and monitors services on its
machine. All agents connect to the **central server** (AgentinoServer),
through which an operator can see the state of the entire infrastructure and
control it from a single window (or from a web browser).

Put simply, Agentino lets you:

* see which programs are running on which computers;
* start and stop them remotely;
* configure their parameters and the connections between them;
* view their logs;
* if necessary, work in the command line of a remote machine,

without connecting to each computer individually.

## 2. What tasks Agentino solves

| Task | How it is done in Agentino |
|------|----------------------------|
| Centralized monitoring of the machine fleet | The **Agents** page shows all agents and their connection state in real time |
| Secure onboarding of new machines | A new agent gets the **Pending** status and starts working only after an operator approves it (**Approve**) |
| Remote start/stop of programs | **Start** / **Stop** commands in the service list, in the topology, or in the service editor |
| Registering a new program on a machine | The **New** command — create a service: executable path, arguments, parameters |
| Automatic start of programs when the machine starts | The **Autostart** option in the service settings |
| Custom start/stop procedures | Custom start and stop scripts (**Start script** / **Stop script**) |
| Configuring connections between services | The **Connections** section: input connections, external ports and choosing a provider for output connections |
| Visual representation of the infrastructure | The **Topology** page — a diagram of all services and the connections between them |
| Diagnostics | Agent logs (**Agent Log**) and service logs (**Service Log**), tracing level |
| Administration of a running service | The **Administration** page in the service editor (for services that support it) |
| Working in the console of a remote machine | The **Terminal** page in the agent editor — a full-featured remote terminal |
| Temporarily disabling or revoking a machine | **Suspend** / **Resume** / **Revoke** commands for agents |

## 3. Key concepts

* **Agentino server (AgentinoServer)** — the central node. Stores the list of
  agents, collects information about their services, and provides the
  operator interface and API.
* **Agent (AgentinoAgent)** — a program installed on a managed computer
  (usually as a system service). It is the "owner" of its services: it stores
  their descriptions, starts and stops them, and monitors their state. The
  agent keeps managing its services even if the connection to the server is
  temporarily lost.
* **Service** — a program (executable) registered on a specific agent. A
  service has a path, command-line arguments, and start-up, logging and
  connection parameters.
* **Connection** — a description of a service's network interaction:
  * *input* — the address/ports on which the service accepts requests;
  * *output* — a service that this service must connect to (for example,
    a database or another server).
* **Topology** — a diagram of all services of all agents and the connections
  between them.
* **Agent registration (Enrollment)** — the procedure for admitting a new
  agent to the server. The agent presents its unique identifier, and an
  operator approves or rejects it.

## 4. System components and access methods

| Component | Where it runs | Purpose |
|-----------|---------------|---------|
| **AgentinoServer** | Central server | Stores data, accepts connections from agents and operators |
| **AgentinoAgent** | Each managed computer | Manages the services of its machine |
| **AgentinoClient** | Operator workstation | Desktop GUI client for working with the server (or directly with an agent) |
| **Web interface** | Operator's browser | The same interface as the client, available in a browser |

Default network ports:

| Side | HTTP | WebSocket |
|------|------|-----------|
| Agentino server | 7111 | 7112 |
| Agent (local interface) | 7222 | 7223 |

> The actual ports may be changed by the administrator during installation.
> Check the address and ports with your system administrator.

## 5. First launch: quick start

Below is the minimal sequence of steps to start managing services on one
remote machine.

1. **Install and start the Agentino server** (done by the administrator).
2. **Install the agent on the managed computer.** On Windows, the
   *AgentinoAgent* installer is used; the agent is registered as a system
   service and starts automatically.
3. **Tell the agent the server address.** Open the agent's local interface
   (see [section 13](#13-agent-local-interface)), enter the server address in
   the **Agentino URL** field and click **Connect**.
4. **Approve the agent on the server.** Sign in to the server interface, open
   the **Agents** page, select the **Pending** filter, select the new agent
   and click **Approve**. (The very first agent may be approved
   automatically.)
5. **Add a service.** On the **Topology** page click **New**, choose the
   target agent, fill in the name and the executable path, and save.
6. **Start the service** with the **Start** command and make sure its status
   has changed to **Running**.

Details for each step are given in the sections below.

## 6. Signing in and access rights

When you open the interface, you may be asked for a login and password
(depending on the configured user management mode). The current user panel is
displayed in the upper right corner of the window.

The set of available pages and commands depends on the rights of your
account. If a button is disabled or a page is missing, you most likely lack
the corresponding right — contact your administrator.

| Group | Rights |
|-------|--------|
| Agent management | View Agents, Add Agent, Edit/Change Agent, Remove Agent, Approve Agents |
| Service management | View Services, Add Service, Edit/Change Service, Remove Service, Start Service, Stop Service |
| Topology | View Topology |
| Remote terminal | RemoteTerminal (granted only to trusted operators) |

## 7. Agentino server interface

The main window consists of:

* **The left navigation panel** with the pages:
  * **Topology** — a diagram of all services;
  * **Agents** — the list of agents.
* **The command panel** (toolbar) above the work area: New, Edit, Remove,
  Start, Stop, Approve, etc. The set of commands depends on the page, and
  their availability depends on the selected item and its state.
* **The tabbed work area.** Opened agents and services open in separate
  tabs, so you can work with several objects at once.

## 8. Working with agents

### 8.1. Agent list

The **Agents** page shows all agents known to the server, regardless of
their registration status. Above the table there is a status filter showing
the number of agents in each:

**All**, **Pending**, **Approved**, **Suspended**, **Rejected**, **Revoked**.

For an approved agent the status column shows the connection state
(**Connected** / **Disconnected**); for others it shows the registration
status.

### 8.2. Agent registration (approval)

Each new agent gets the **Pending** status (awaiting a decision) on its first
connection. Until the agent is approved, it cannot be managed.

Select the agent in the list and use the toolbar commands:

| Command | When available | Result |
|---------|----------------|--------|
| **Approve** | Pending status | The agent is admitted to work |
| **Reject** | Pending status | The agent is rejected |
| **Suspend** | Approved status | The agent's operation is temporarily suspended |
| **Resume** | Suspended status | The agent's operation is resumed |
| **Revoke** | Approved or Suspended | The agent's admission is revoked |
| **Reset** | Rejected or Revoked | The decision is reset; the agent can register again |

> Approve only agents you expect to see. An unknown agent in the Pending
> status is a reason to check who connected it and from where.

### 8.3. Agent editor

Double-clicking an approved agent (or the **Edit** command) opens the agent
editor in a separate tab. The editor contains these pages:

* **General** — the agent's name and description, verbose messages mode
  (**Verbose message**) and tracing level (**Tracing level**);
* **Services** — the list of this agent's services (the page title shows
  their number);
* **Agent Log** — the agent's log (a dot next to the title means new entries
  have appeared);
* **Terminal** — the remote terminal (only with the RemoteTerminal right, see
  [section 12](#12-remote-terminal)).

To save changes, use the save command on the editor toolbar.

### 8.4. Removing an agent

Select the agent, run **Remove**, then confirm the removal in the dialog.

## 9. Working with services

You can work with services from three places:

* the **Services** page in the agent editor — the list of services of a
  specific machine;
* the **Topology** page — all services on a single diagram;
* the agent's local interface.

### 9.1. Creating a service

1. On the **Topology** page click **New** (or **New** on the **Services**
   page of the agent editor).
2. On the server the **Select target agent** dialog appears — choose the agent
   on which the service will run and click **Select**.
3. The new service editor opens. Fill in at least **Name** and **Path** and
   save the service.

### 9.2. Service editor

The editor opens by double-clicking a service, with the **Edit** command, or
with the **Enter** key on the selected topology item. The upper part shows
the current service status (Running, Stopped, Starting, Stopping, Unknown, or
*Not saved yet* for an unsaved service).

#### General → Information

| Field | Description |
|-------|-------------|
| **Name** | Service name; used in lists, the topology and tab titles. Required |
| **Description** | Description of the service's purpose |
| **Path** | Path to the executable on the agent machine. The service is started from it and its running process is detected by it. Required. The browse button lets you choose a file in the agent's file system |
| **Arguments** | Command-line arguments passed to the executable |

#### General → Options

| Field | Description |
|-------|-------------|
| **Autostart** | Start the service automatically when the agent starts |
| **Verbose Message** | Enable diagnostic (tracing) messages of the service |
| **Tracing level** | Amount of diagnostic information produced |
| **Start script** / **Start Script Path** | Use the specified script to start the service instead of launching the executable directly |
| **Stop script** / **Stop Script Path** | Use the specified script to stop the service |

#### Connections → Server (input connections)

After the service is saved, Agentino automatically loads information about
the connections it supports (status *Plugin loaded*). If loading fails, check
the executable path and click **Reload**.

For each input connection you can click **Edit** next to **Extern
Connections** and, in the **Edit ports** dialog, specify externally
accessible addresses: **Host**, **Http Port**, **Web Socket Port**,
**Http Path**, **Security**.

#### Connections → Output Connections

Here you specify which other services this service should connect to. Each
connection shows **Service Type** (the required service type) and
**Connection Name**. In the **Available Connections** list choose a suitable
provider service — it can run on any agent.

If there are no suitable providers, a message tells you that you first need
to add a service of the required type on any agent (it does not need to be
running for that).

#### Administration

Appears for services that support their own administration. Lets you manage
the internal settings of a running service. If the service is not running,
the message *Service not running* is shown.

#### Service Log

The log of the selected service (available after the service is saved), see
[section 11](#11-logs).

### 9.3. Starting and stopping

* **Start** — available when the service is stopped (Stopped).
* **Stop** — available when the service is running (Running).

During the operation an indicator *Starting service...* /
*Stopping service...* is shown. If the editor has unsaved changes, you will
be offered to save them when starting (*This service has unsaved changes.
Save and start it?*).

If the agent is disconnected, the service status is unknown and the
start/stop commands are unavailable.

### 9.4. Removing a service

Select the service, run **Remove**, then confirm the removal. A service can
be removed only while its agent is connected — otherwise the message
*Cannot delete the service while the agent is disconnected* appears.

## 10. Topology

The **Topology** page displays all services as a diagram: each service is a
separate block, and connections between services are lines between blocks.

Features:

* **Selecting a service** with a mouse click — a panel with detailed
  information about it opens on the right (including external connection
  addresses).
* **Moving blocks** with the mouse — the layout is saved automatically.
* **AutoFit** — automatically fit the diagram to the window.
* **New / Edit / Remove / Start / Stop** — the same actions as in the service
  list. **Enter** opens the editor of the selected service, **Delete** removes
  it (with confirmation).
* **Real-time updates** — service statuses and composition changes are
  updated without reloading the page.

Services of disconnected agents remain on the diagram with their last known
state and are marked with warning icons; they cannot be managed until the
connection is restored.

> The block layout on the server diagram and on the diagram in the agent's
> local interface are stored independently.

## 11. Logs

* **Agent Log** (agent editor) — messages of the agent itself.
* **Service Log** (service editor) — messages of the selected service.

Logs are updated automatically. To get more diagnostic information, enable
**Verbose message** and increase **Tracing level** in the agent or service
settings.

## 12. Remote terminal

The remote terminal lets you work in the command line of the computer where
the agent is installed, directly from the Agentino interface — just like in a
regular terminal window.

> ⚠️ The terminal gives full command execution access on the remote machine
> with the privileges of the agent service. Only users with the
> **RemoteTerminal** right have access to it.

### How to open

1. On the **Agents** page open an approved agent (double-click or **Edit**).
2. Go to the **Terminal** page.
3. Click **+** (or **Ctrl+Shift+T**) — a new tab opens with the default shell
   (Command Prompt on Windows, Bash on Linux). The **▾** button lets you
   choose another shell (for example, PowerShell or sh) — only shells
   actually available on the agent are offered.

### Working in the terminal

| Action | How to do it |
|--------|--------------|
| Run a command | Click the output area, type the command right after the prompt and press **Enter** |
| Command history | **↑ / ↓** keys (provided by the shell) |
| Paste text | **Ctrl+V** or **Shift+Insert** |
| Interrupt the current command | **Ctrl+C** (the session is not closed; with selected text it copies instead) |
| Clear the screen | **Clear** |
| Copy all output | Copy-to-clipboard button |
| Save output to a file | **Save terminal log** (text file) |
| Close the session | The tab's close button |
| Restore a lost output channel | **Reconnect** |

Colored output and full-screen programs (for example, `vim`, `mc`, `top`)
are supported.

### Limitations

* You can open several tabs, but the number of concurrent sessions per agent
  is limited.
* An inactive session is closed automatically after about **15 minutes**; a
  warning is shown about a minute before closing.
* Sessions are bound to the agent editor tab and are closed together with it.
* Commands are executed with the privileges of the agent service, without
  privilege elevation.

## 13. Agent local interface

Each agent has its own interface (by default `http://<agent-address>:7222`).
It is useful for initial setup and for working with the machine when the
central server is unavailable.

Pages of the local interface:

* **Settings** — connection to the server:
  * **Status** — current state of the connection to the server: *Connected*
    (green), *Disconnected* (red), *Unknown* (yellow),
    *Checking the connection*;
  * **Agentino URL** — the Agentino server address; after entering it click
    **Connect**.
* **Topology** — the diagram and management of this agent's services
  (create, edit, start, stop, remove) without choosing a target agent.

## 14. Typical scenarios

### Connect a new computer

1. Install AgentinoAgent on the computer.
2. In the agent's local interface, enter the **Agentino URL** and click
   **Connect**.
3. On the server: **Agents** → **Pending** filter → select the agent →
   **Approve**.
4. Make sure the agent's status is **Connected**.

### Deploy and start an application on a remote machine

1. Copy the application to the agent machine (for example, using
   administration tools or the terminal).
2. **Topology** → **New** → choose the agent → enter **Name**, **Path**, and
   **Arguments** if needed.
3. Enable **Autostart** if the application should start together with the
   agent.
4. Save and click **Start**.

### Connect two services

1. Make sure the provider service (for example, a database) has already been
   added.
2. Open the consumer service → **Connections → Output Connections**.
3. In **Available Connections** choose the provider service and save.
4. The connection appears on the **Topology** diagram.

### Find out why a service is not working

1. Find the service on **Topology** and check its status and the agent's
   status.
2. Open **Service Log**; if needed, enable **Verbose Message**, increase
   **Tracing level** and restart the service.
3. Check the **Agent Log** of the corresponding agent.
4. If necessary, open **Terminal** and check the environment (presence of
   files, ports in use, etc.).

### Temporarily take a machine out of operation

* **Suspend** — temporarily suspend, **Resume** — bring it back.
* **Revoke** — permanently revoke admission (it can be restored via
  **Reset** and re-approval).

## 15. Troubleshooting

| Symptom | Possible cause and solution |
|---------|-----------------------------|
| A new agent does not appear in the list | Check the **Agentino URL** in the agent's local interface, network reachability of the server and the ports (7111/7112 by default) |
| Agent in Pending status | The agent is awaiting approval — run **Approve** |
| Cannot open an agent (Edit is disabled) | The agent is not yet approved, is suspended or rejected |
| Agent in Disconnected status | The agent service is stopped or there is no network connection to the server |
| Start/Stop are disabled | The agent is disconnected (service status unknown) or you lack the rights |
| *Cannot delete the service while the agent is disconnected* | Restore the connection to the agent and retry the removal |
| *Failed to load plugin* | Wrong **Path** or the file is not accessible on the agent machine — fix the path and click **Reload** |
| No suitable **Available Connections** | A service of the required type has not yet been added on any agent |
| No **Terminal** page | No RemoteTerminal right or the agent is not approved |
| *No shell available* | The agent found no supported shell |
| *Output subscription lost* in the terminal | Click **Reconnect** or open a new session |
| *[session closed: idle timeout]* | The session was closed due to inactivity — open a new tab |

## 16. Status reference

### Agent registration statuses

| Status | Meaning |
|--------|---------|
| **Pending** | Awaiting the operator's decision |
| **Approved** | Admitted to work |
| **Suspended** | Temporarily suspended |
| **Rejected** | Rejected |
| **Revoked** | Admission revoked |

### Agent connection statuses (for approved agents)

| Status | Meaning |
|--------|---------|
| **Connected** | The agent is online |
| **Disconnected** | No connection to the agent |
| **Undefined / Unknown** | The state has not been determined yet |

### Service statuses

| Status | Meaning |
|--------|---------|
| **Running** | The service is running |
| **Stopped** | The service is stopped |
| **Starting** | Starting is in progress |
| **Stopping** | Stopping is in progress |
| **Unknown** | The state is unknown (usually the agent is disconnected) |
| **Not saved yet** | The service has been created but not saved yet |
