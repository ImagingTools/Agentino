# Agentino GUI tests

End-to-end tests for Agentino's web clients (Qt/QML compiled to JavaScript), driven with Playwright.

One app is under test: **AgentinoServerTest.exe** (default port `17111`), and within it exactly two
pages - the **Agents** page (the enrollment workflow, the agent editor, and an agent's own **Services**
inside it) and the **Topology** page. That is what `playwright.config.js`'s `baseUrl`/session fixtures
point at.

Out of scope, both deliberately:

* **Administration** (Roles/Users/Groups) - this server does not implement the document service its
  editors need, answering `Invalid command request: 'CreateNewDocument'`.
* **AgentinoAgentTest.exe's own web UI** - a separate app, and its client is not deployed. The agent
  PROCESS is still started on every run, because the suite needs it to enroll; see below.

Built on **`imtcore-gui-testkit`** (`ImtCore/Tests/GuiTestKit`), the framework extracted from ProLife's
GUI suite. Everything generic - the objectName locator engine, the collection/editor page-object base
classes, the fixture/global-setup/config factories - lives there. What is here is only Agentino's own:
its Agent/Service editors, its Topology page, its fixture user, and its CI script.

```
AgentinoGui/
  fixtures/users.js        the fixture users (su only - see fixtures/users.js for why)
  fixtures/test.js          the test/expect/gui/newUserPage bundle
  fixtures/refuse.js        refuse(what) - loud failure instead of a silent skip for su-should-see-this
  global-setup.js           logs each fixture user in once and saves its storageState
  playwright.config.js      projects, timeouts, snapshot paths, worker count
  pages/                    Agentino's own page objects (Agents/Services/Topology)
  tests/                    the specs
  Run-CiTests.ps1           start the server -> bootstrap su -> run both phases -> tear down
```

## Running it

The whole thing, against a fresh server:

```powershell
.\Run-CiTests.ps1
```

That starts `AgentinoServerTest.exe` and `AgentinoAgentTest.exe`, bootstraps the `su` superuser via a
`CreateSuperuser` GraphQL mutation, runs Playwright in two phases, and stops both applications again.

Against a server that is already running (the fast loop while writing a test):

```powershell
$env:AGENTINO_GUI_BASE_URL = "http://localhost:7111"
npx playwright test tests/agents.collection.test.js
npx playwright test --update-snapshots          # (re)generate the screenshot baselines
```

## No seeded data

Unlike Lisa/ProLife, Agentino has no Puma dependency and no database backup this suite restores before
a run. Agents and Services are not something the UI can create on its own - a row only exists once a
real `AgentinoAgent` process enrolls with the server. So every spec that needs a row of a particular
kind or status (an Approved agent to edit, a Rejected one to Reset, a service to open...) asks the
table for one first and **skips**, not fails, when this instance does not currently have one. See
`fixtures/refuse.js` for the distinction this suite draws throughout: something the `su` superuser
(permissions `['*']`) should always be able to reach (a page, a command bar button) is a hard failure
if missing; whether a particular row of live data happens to exist is not.

This also means the suite's coverage of the enrollment workflow (Approve/Reject/Suspend/Resume/Revoke)
and of Service/Topology editing is only as good as whatever agents happen to be connected to the
instance a run targets - point it at an instance with a real Agent enrolled for full coverage.

## The two phases

Every test runs as the same fixture user against one shared server, so a test that MUTATES data (Reset,
Connect, ...) changes state other tests are looking at. `Run-CiTests.ps1` therefore invokes Playwright
twice against the one running server:

* **phase 1** - everything except `@mutating`
* **phase 2** - only `@mutating`, `--workers=1`

The suite passes only if both do. `playwright.config.js` also pins `workers: 1` overall, since there is
no restricted second fixture user here to spread sessions across (see `fixtures/users.js`'s own note).

Everything a run writes goes under one directory:

    test-output/
      phase1-readonly/   artifacts/ (only for failures) + junit.xml
      phase2-mutating/   artifacts/ + junit.xml

Per PHASE because Playwright clears its output dir and truncates its junit file at the start of every
invocation, so one shared pair would let phase 2 wipe phase 1's evidence.

## AgentinoAgent is run, but not tested

`Run-CiTests.ps1` starts `AgentinoAgentTest.exe` on every run and that is not optional: the agent has to
connect and enroll, because `Approve-TestAgent` then approves it and the whole Agents/Services suite
works against that one Approved agent.

Its own web UI is **not** covered. It is a separate app with its own independent auth, and its web
client is not currently deployed anyway - the process answers `Unable to open file
html/indexAgent.html`, because `Include/agentinoqml/CMake/CMakeLists.txt` builds that client with
`dataroot "/Agent/Views/"` (producing `index.html` at the root) while
`AgentinoAgentVoce.arp/Handlers.acc` redirects to `Agent/Views/html/indexAgent.html`.
