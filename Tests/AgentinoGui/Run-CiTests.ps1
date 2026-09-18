# SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-ImtCore-Commercial
<#
.SYNOPSIS
    Runs the Agentino GUI (Playwright) suite end to end on a CI agent.

.DESCRIPTION
    Starts AgentinoServerTest.exe and AgentinoAgentTest.exe, bootstraps the
    "su" superuser, runs Playwright in two phases, and tears both apps down.

    The suite starts isolated Agentino test binaries and creates its own service
    rows during the @mutating workflow. Lisa/Puma executables are only stored as
    service paths; they are not launched by this runner.

    Why two phases: every test runs as the same fixture user against one
    shared server, so a test that MUTATES data (Reset, Connect, ...) changes
    state other tests are looking at. Phase 1 runs everything except
    @mutating, phase 2 runs only @mutating, serially. The suite passes only
    if both do.

.PARAMETER PlaywrightArgs
    Appended verbatim to "npx playwright test" - e.g. a spec path to scope
    the run ('tests/topology.test.js'), or --update-snapshots to (re)generate
    baselines.
#>
param(
    # Resolved from AGENTINODIR when that points at a real checkout, then the
    # working directory, then this script's own location - so it works both
    # from TeamCity (which sets the env var) and from a developer shell.
    [string]$RepoRoot = $(
        $configRelPath = "Tests\AgentinoGui\playwright.config.js"
        if ($env:AGENTINODIR -and (Test-Path (Join-Path $env:AGENTINODIR $configRelPath))) {
            $env:AGENTINODIR
        }
        elseif (Test-Path (Join-Path (Get-Location).Path $configRelPath)) {
            (Get-Location).Path
        }
        else {
            $sd = if ($PSScriptRoot) { $PSScriptRoot }
                  elseif ($PSCommandPath) { Split-Path -Parent $PSCommandPath }
                  elseif ($MyInvocation.MyCommand.Path) { Split-Path -Parent $MyInvocation.MyCommand.Path }
                  else { $null }
            if ($sd) {
                (Resolve-Path (Join-Path $sd "..\..")).Path
            }
            else {
                throw "Unable to determine the Agentino checkout root (AGENTINODIR unset/stale, working directory isn't the checkout root, and this script's own path could not be determined). Pass -RepoRoot explicitly."
            }
        }
    ),
    [string]$ScriptDir = (Join-Path $RepoRoot "Tests\AgentinoGui"),

    # Release, matching TeamCity (which builds only Release) and the other suites' CI scripts. Locally:
    # -BuildConfig Debug_Qt6_VC17_x64.
    [string]$BuildConfig = "Release_Qt6_VC17_x64",

    [string]$AgentinoServerExePath = "",
    [string]$AgentinoAgentExePath = "",

    # The two checkouts whose servers this suite drives as Agentino SERVICES. Same convention the rest
    # of the toolchain already uses (TeamCity sets these; a developer shell usually has them too), with
    # the sibling-checkout layout as the fallback.
    [string]$LisaRepoRoot = $(if ($env:LISADIR) { $env:LISADIR } else { Join-Path (Split-Path -Parent $RepoRoot) "Lisa" }),
    [string]$PumaRepoRoot = $(if ($env:PUMADIR) { $env:PUMADIR } else { Join-Path (Split-Path -Parent $RepoRoot) "Puma" }),

    [string]$LisaServiceExePath = "",
    [string]$PumaServiceExePath = "",

    # ServerSettings.acc's DefaultHttpPort.
    [int]$HttpPort = 17111,
    [int]$AgentGuiPort = 17222,

    # Safety net only. The login is always "su" (hardcoded here and in
    # fixtures/users.js); only the password is configurable.
    [string]$SuPassword = "AgentinoGuiTest1!",

    # The identity the test agent enrolls and routes under - see Initialize-AgentinoTestAgentSettings
    # for why it must not be empty.
    [string]$AgentClientId = "a9e7c341-d5b4-4f28-9c61-0f17e5c0a901",

    # Everything a run writes lives under ONE directory, one subfolder per phase - see Lisa's own
    # Run-CiTests.ps1 for why per-phase: Playwright clears its output dir at the start of every
    # invocation, so one shared pair would let phase 2 wipe phase 1's evidence.
    [string]$OutputRoot = (Join-Path $ScriptDir "test-output"),
    [int]$StartupTimeoutSeconds = 120,

    [string[]]$PlaywrightArgs = @()
)

$ErrorActionPreference = "Stop"
$serverProcess = $null
$agentProcess = $null
$exitCode = 1

if ([string]::IsNullOrWhiteSpace($AgentinoServerExePath)) {
    $AgentinoServerExePath = Join-Path $RepoRoot "Bin\$BuildConfig\AgentinoServerTest.exe"
}
if ([string]::IsNullOrWhiteSpace($AgentinoAgentExePath)) {
    $AgentinoAgentExePath = Join-Path $RepoRoot "Bin\$BuildConfig\AgentinoAgentTest.exe"
}
if ([string]::IsNullOrWhiteSpace($LisaServiceExePath)) {
    $LisaServiceExePath = Join-Path $LisaRepoRoot "Bin\$BuildConfig\LisaServerTest.exe"
}
if ([string]::IsNullOrWhiteSpace($PumaServiceExePath)) {
    $PumaServiceExePath = Join-Path $PumaRepoRoot "Bin\$BuildConfig\PumaServerPgTest.exe"
}

function Write-Step($message) {
    Write-Host "`n=== $message ===" -ForegroundColor Cyan
}

function Stop-ServerProcess([string]$processName) {
    Write-Step "Stopping any running $processName.exe"
    Get-Process -Name $processName -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "Killing PID $($_.Id)"
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 2
}

function Wait-ForPort([string]$serverLabel, [System.Diagnostics.Process]$process, [int]$port) {
    $deadline = (Get-Date).AddSeconds($StartupTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($process.HasExited) {
            throw "$serverLabel exited prematurely (exit code $($process.ExitCode))"
        }
        $probe = Test-NetConnection -ComputerName "localhost" -Port $port -WarningAction SilentlyContinue -InformationLevel Quiet
        if ($probe) { return }
        Start-Sleep -Milliseconds 500
    }
    throw "$serverLabel did not open port $port within $StartupTimeoutSeconds seconds"
}

function Reset-AgentinoTestState {
    # Lisa and ProLife get a deterministic starting point by restoring their databases; Agentino has
    # no database, it persists to XML/SQLite under the shared company folder
    # (C:/Users/Public/ImagingTools - see Initialize-AgentinoTestAgentSettings for why that path).
    # That state outlives a run, and two things in it break the suite outright:
    #   - agentino.sqlite holds the "su" account, so CreateSuperuser answers "Superuser already
    #     exists" and the password this script bootstraps with is NOT the one that account has -
    #     every UI login then times out and the whole run dies in global-setup.
    #   - AgentEnrollment.xml accumulates agents from earlier runs, including long-dead ones still
    #     marked Approved, so "the first Approved agent" is whichever zombie sorts first.
    #
    # Everything removed is copied to test-output/state-backup first: this is shared, user-visible
    # data, not a scratch directory, and the production "Agentino Agent"/"Agentino Server" folders
    # are deliberately NOT touched.
    Write-Step "Resetting persisted test-server state"
    $companyDirectory = Join-Path $env:PUBLIC "ImagingTools\Agentino"
    $backupDirectory = Join-Path $OutputRoot "state-backup"
    New-Item -ItemType Directory -Path $backupDirectory -Force | Out-Null

    $staleItems = @(
        (Join-Path $companyDirectory "Agentino Server Test\agentino.sqlite"),
        (Join-Path $companyDirectory "Agentino Server Test\AgentCollection.xml"),
        (Join-Path $companyDirectory "Agentino Server Test\TopologyCollection.xml"),
        (Join-Path $companyDirectory "Agentino Server\AgentEnrollment.xml"),
        (Join-Path $companyDirectory "Agentino Agent Test\ServicesSettings.xml"),
        (Join-Path $companyDirectory "Agentino Agent Test\TopologyCollection.xml")
    )
    foreach ($item in $staleItems) {
        if (-not (Test-Path $item)) { continue }
        Copy-Item -Path $item -Destination (Join-Path $backupDirectory (Split-Path -Leaf $item)) -Force -ErrorAction SilentlyContinue
        Remove-Item -Path $item -Force -ErrorAction SilentlyContinue
        Write-Host "  reset $(Split-Path -Leaf $item)"
    }
    Write-Host "Previous contents backed up to $backupDirectory"
}

function Start-AgentinoTestServer {
    Write-Step "Starting AgentinoServerTest.exe"
    if (-not (Test-Path $AgentinoServerExePath)) {
        throw "Server executable not found: $AgentinoServerExePath"
    }
    $workDir = Split-Path -Parent $AgentinoServerExePath
    $script:serverProcess = Start-Process -FilePath $AgentinoServerExePath -WorkingDirectory $workDir -PassThru -WindowStyle Hidden
    Write-Host "Started PID $($script:serverProcess.Id)"
    Wait-ForPort "AgentinoServerTest.exe" $script:serverProcess $HttpPort
    Write-Host "Agentino test server is accepting connections on port $HttpPort"
}

function Initialize-AgentinoTestAgentSettings {
    # NOT next to the exe. The agent's settings component is ifile::CSystemLocationComp with
    # LocationType 104 = SL_SHARED_COMPANY_DIRECTORY, which on Windows resolves to
    # "C:/Users/Public/<CompanyName>" (CompanyName is "ImagingTools", from the .acc's Application
    # element) - so this is the ONLY path the agent reads, and a file written anywhere else is
    # silently ignored. FileAutoPersistence loads it at startup and it OVERRIDES the ports compiled
    # into AgentinoAgentTest.acc, which is how a stale file pinned the agent to 7111/7112 while the
    # test server listened on 17111/17112 and nothing ever enrolled.
    $companyDirectory = Join-Path $env:PUBLIC "ImagingTools"
    $settingsPath = Join-Path $companyDirectory "Agentino\Agentino Agent\AgentinoAgentTestSettings.xml"
    $settingsDirectory = Split-Path -Parent $settingsPath
    New-Item -ItemType Directory -Path $settingsDirectory -Force | Out-Null

    # AgentinoWebSocketUrl is NOT a URL parameter despite the name: AgentinoVoce.arp/ServerSettings.acc
    # declares it as a ServerConnectionInterfaceParam, so it serialises as Host/ConnectionFlags/
    # Interfaces/Paths - the shape the agent itself writes back. A <Parameter ... Url="..."/> form
    # parses without error and is then ignored.
    #
    # ClientId MUST be set. Leaving it empty leaves the agent permanently "Disconnected" and its
    # service list permanently empty, because the two code paths that need an agent id disagree about
    # what an empty one means:
    #   - CAgentRegistrationClientComp::ResolveAgentId falls back to the HOST NAME, so the agent
    #     enrolls as e.g. "LAPTOP";
    #   - CWebSocketClientComp::BuildRequestEnvelope omits the "clientid" field from the envelope
    #     ENTIRELY when it is empty, so the server registers the socket's sender under no id.
    # The server then routes by the enrollment id and finds nothing:
    #     No WebSocket sender registered for clientid 'LAPTOP' (client offline or id mismatch)
    #     Request could not be sent: 'ServicesList'
    # A deployed agent never hits this because it has a persisted ClientId - which is exactly why this
    # only ever went wrong in the test environment. Verified both ways: empty -> Disconnected, set ->
    # Connected.
    #
    # A fixed value rather than a new GUID per run: the enrollment store is reset before every run
    # anyway, so nothing accumulates, and a stable id keeps the server log readable across runs.
    # The AcfHeader is NOT decoration: without it the ACF archive reader refuses the file outright
    # ("Cannot open file: ..." in the agent's log), falls back to the ports compiled into the registry,
    # and nothing ever enrolls - with no error that names the settings file as the cause. These are the
    # version numbers this build's agent writes back itself; an existing file's own header is preferred
    # over them below.
    $defaultHeader = @"
    <AcfHeader>
        <VersionInfos>
            <Version Id="0" Number="6583" Description="ACF"/>
            <Version Id="1" Number="2449" Description="ACF-Solutions"/>
            <Version Id="10" Number="986" Description="IACF"/>
            <Version Id="1023" Number="395264" Description="Qt Framework"/>
            <Version Id="1977" Number="23257" Description="ImtCore"/>
            <Version Id="2022" Number="549" Description="ServiceManager"/>
        </VersionInfos>
    </AcfHeader>
"@
    # Prefer the header the agent last wrote, so the versions stay in step with the binary as it moves.
    $header = $defaultHeader
    if (Test-Path $settingsPath) {
        $existing = [System.IO.File]::ReadAllText($settingsPath)
        $match = [regex]::Match($existing, '(?s)<AcfHeader>.*?</AcfHeader>')
        if ($match.Success) { $header = "    " + $match.Value }
    }

    $xml = @"
<?xml version="1.0" encoding="UTF-8"?>
<Acf>
$header
    <Parameters>
        <Parameter Id="Agent">
            <Parameters>
                <Parameter Id="ClientId" Text="$AgentClientId"/>
            </Parameters>
        </Parameter>
        <Parameter Id="AgentinoServer">
            <Parameters>
                <Parameter Id="AgentinoWebSocketUrl">
                    <Host>localhost</Host>
                    <ConnectionFlags>default</ConnectionFlags>
                    <Interfaces>
                        <Interface Protocol="http" Port="$HttpPort"/>
                        <Interface Protocol="websocket" Port="$($HttpPort + 1)"/>
                    </Interfaces>
                    <Paths/>
                </Parameter>
            </Parameters>
        </Parameter>
        <Parameter Id="AgentServer">
            <Parameters>
                <Parameter Id="ServerConnectionInterface">
                    <Host>localhost</Host>
                    <ConnectionFlags>default</ConnectionFlags>
                    <Interfaces>
                        <Interface Protocol="http" Port="$AgentGuiPort"/>
                        <Interface Protocol="websocket" Port="$($AgentGuiPort + 1)"/>
                    </Interfaces>
                    <Paths/>
                </Parameter>
            </Parameters>
        </Parameter>
    </Parameters>
</Acf>
"@
    # WriteAllText with an explicit BOM-less encoding: Set-Content -Encoding UTF8 emits a BOM on
    # Windows PowerShell 5.1.
    [System.IO.File]::WriteAllText($settingsPath, $xml, (New-Object System.Text.UTF8Encoding($false)))
    Write-Host "Initialized test-agent settings: $settingsPath"
}

function Start-AgentinoTestAgent {
    Write-Step "Starting AgentinoAgentTest.exe"
    if (-not (Test-Path $AgentinoAgentExePath)) {
        throw "Agent executable not found: $AgentinoAgentExePath"
    }
    Initialize-AgentinoTestAgentSettings
    $workDir = Split-Path -Parent $AgentinoAgentExePath
    $script:agentProcess = Start-Process -FilePath $AgentinoAgentExePath -WorkingDirectory $workDir -PassThru -WindowStyle Hidden
    Write-Host "Started PID $($script:agentProcess.Id)"
    Wait-ForPort "AgentinoAgentTest.exe" $script:agentProcess $AgentGuiPort
    Write-Host "Agentino test UI is accepting connections on port $AgentGuiPort"
}

function New-SuperuserIfNeeded {
    Write-Step "Bootstrapping 'su' superuser via CreateSuperuser"
    $body = @{
        query = 'mutation CreateSuperuser { CreateSuperuser(input: { password: "' + $SuPassword + '", mail: "su@agentinogui.test", name: "Super User" }) { success message } }'
    } | ConvertTo-Json -Compress

    $uri = "http://localhost:$HttpPort/Agentino/graphql"

    # Retried - an open port is not the same as a usable server; see Lisa's own Run-CiTests.ps1 for
    # the measured "answers with no payload for a moment after the port opens" behaviour this guards.
    $attempts = 10
    for ($attempt = 1; $attempt -le $attempts; $attempt++) {
        $response = Invoke-RestMethod -Uri $uri -Method Post -ContentType "application/json" -Body $body

        $refusals = @($response.errors | Where-Object { $_ -and $_.extensions.type -ne "Warning" })
        if ($refusals.Count -gt 0) {
            throw "CreateSuperuser GraphQL call failed: $($refusals | ConvertTo-Json -Compress)"
        }

        $result = $response.data.CreateSuperuser
        if ($null -ne $result) {
            if (-not $result.success -and $result.message -notmatch "already exists") {
                throw "CreateSuperuser did not succeed: $($result.message)"
            }
            Write-Host "CreateSuperuser: $($result.message)"
            return
        }

        if ($attempt -lt $attempts) {
            Write-Host "CreateSuperuser has no payload yet (attempt $attempt/$attempts) - the server is still wiring up; retrying"
            Start-Sleep -Seconds 3
        }
    }

    throw "CreateSuperuser returned no payload after $attempts attempts: $($response | ConvertTo-Json -Compress)"
}

function Approve-TestAgent {
    # Agentino's equivalent of Lisa restoring a database: give the read-only phase a deterministic
    # Approved agent to work against. Without this the freshly-enrolled agent sits in Pending, every
    # Agents/Services/Topology spec answers "no Approved agent is currently enrolled" and skips, and
    # the run goes green having tested nothing.
    #
    # Driven over GraphQL rather than through the UI on purpose: the UI path IS what agents.collection
    # covers, and seeding through the thing under test would make a broken Approve look like healthy
    # fixture data.
    Write-Step "Approving the test agent (enrollment seeding)"
    $uri = "http://localhost:$HttpPort/Agentino/graphql"

    $authBody = @{ query = 'query Authorization { Authorization(input: { login: "su", password: "' + $SuPassword + '" }) { token } }' } | ConvertTo-Json -Compress
    $token = (Invoke-RestMethod -Uri $uri -Method Post -Body $authBody -ContentType "application/json").data.Authorization.token
    if (-not $token) { throw "Could not authorize as 'su' to approve the test agent" }
    $headers = @{ Authorization = "Bearer $token" }

    # The agent connects a few seconds after its port opens, and enrollment is what creates the record.
    #
    # Matched on $AgentClientId rather than taking items[0]: if the enrollment reset ever fails, a
    # leftover record from an older run would be approved instead, and the suite would then run against
    # an agent that is enrolled but permanently Disconnected - the exact failure this id exists to
    # prevent, and a confusing one to debug from the test side.
    $pendingBody = @{ query = 'query { PendingAgentsList { items { agentId status } } }' } | ConvertTo-Json -Compress
    $agentId = $null
    for ($attempt = 1; $attempt -le 30; $attempt++) {
        $items = (Invoke-RestMethod -Uri $uri -Method Post -Body $pendingBody -ContentType "application/json" -Headers $headers).data.PendingAgentsList.items
        $match = @($items | Where-Object { $_.agentId -eq $AgentClientId })
        if ($match.Count -gt 0) { $agentId = $match[0].agentId; break }
        Start-Sleep -Seconds 2
    }
    if (-not $agentId) {
        throw "Agent '$AgentClientId' did not reach Pending within 60s. Check the agent log for 'Cannot open file: ...AgentinoAgentTestSettings.xml' (a settings file without an AcfHeader is rejected and the agent then uses its compiled-in ports). If a record enrolled under the MACHINE NAME instead, the settings file reached the agent without a ClientId."
    }

    $approveBody = @{ query = "mutation { ApproveAgent(input: { agentId: `"$agentId`", note: `"approved by Run-CiTests`" }) { successful agentId } }" } | ConvertTo-Json -Compress
    $result = (Invoke-RestMethod -Uri $uri -Method Post -Body $approveBody -ContentType "application/json" -Headers $headers).data.ApproveAgent
    if (-not $result.successful) { throw "ApproveAgent did not succeed for '$agentId'" }
    Write-Host "Approved agent '$agentId'"
}

function Install-PlaywrightIfNeeded {
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        if (-not (Test-Path (Join-Path $ScriptDir "node_modules\@playwright\test"))) {
            Write-Step "Installing Playwright (npm install in $ScriptDir)"
            Push-Location $ScriptDir
            try {
                & npm install --no-audit --no-fund 2>&1 | Out-Host
                if ($LASTEXITCODE -ne 0) { throw "npm install failed (exit $LASTEXITCODE)" }
            }
            finally {
                Pop-Location
            }
        }

        Write-Step "Ensuring Playwright's Chromium browser is installed"
        Push-Location $ScriptDir
        try {
            & npx playwright install chromium 2>&1 | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "playwright install failed (exit $LASTEXITCODE)" }
        }
        finally {
            Pop-Location
        }
    }
    finally {
        $ErrorActionPreference = $previousEap
    }
}

function Sync-GuiTestKit {
    # imtcore-gui-testkit is a "file:" devDependency, which npm COPIES into node_modules rather than
    # symlinking (see .npmrc's install-links=true). Mirror it before every run so a kit edit is
    # picked up without needing a version bump - see Lisa's own Run-CiTests.ps1 for the full rationale.
    $kitSource = Join-Path $RepoRoot "..\ImtCore\Tests\GuiTestKit"
    if (-not (Test-Path $kitSource)) {
        Write-Host "Sync-GuiTestKit: source not found at $kitSource - skipping (using node_modules copy as-is)"
        return
    }
    $kitDest = Join-Path $ScriptDir "node_modules\imtcore-gui-testkit"

    Write-Step "Syncing imtcore-gui-testkit into node_modules (file: dependency is copied, not symlinked)"
    & robocopy $kitSource $kitDest /MIR /NFL /NDL /NJH /NJS /XD node_modules | Out-Host
    $robocopyExit = $LASTEXITCODE
    if ($robocopyExit -ge 8) {
        throw "Sync-GuiTestKit: robocopy failed copying $kitSource -> $kitDest (exit code $robocopyExit)"
    }
}

function Invoke-PlaywrightSuite {
    Install-PlaywrightIfNeeded
    Sync-GuiTestKit

    & node (Join-Path $ScriptDir "node_modules\imtcore-gui-testkit\scripts\prepare-output.js") $OutputRoot | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Failed to prepare GUI test output (exit $LASTEXITCODE)" }

    Write-Step "Running Playwright suite"
    Push-Location $ScriptDir
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $env:CI = "true"
        $env:AGENTINO_GUI_BASE_URL = "http://localhost:$HttpPort"
        # Not read by any spec today - the create/link/start-stop scenarios that need them are
        # deferred (a service path cannot be typed; it comes from the Browse dialog). Kept exported so
        # that work does not have to re-plumb them, and because these are the LISADIR/PUMADIR-resolved
        # paths the harness already validates and prints.
        $env:AGENTINO_LISA_SERVICE_PATH = $LisaServiceExePath
        $env:AGENTINO_PUMA_SERVICE_PATH = $PumaServiceExePath
        $env:PLAYWRIGHT_OUTPUT_ROOT = $OutputRoot
        try {
            $env:PLAYWRIGHT_OUTPUT_PHASE = "phase1-readonly"
            Write-Step "Playwright phase 1/2: read-only tests"
            & npx playwright test @PlaywrightArgs --grep-invert '@mutating' | Out-Host
            $phase1 = $LASTEXITCODE

            $env:PLAYWRIGHT_OUTPUT_PHASE = "phase2-mutating"
            Write-Step "Playwright phase 2/2: @mutating tests"
            # global-setup runs again on this second invocation (Playwright keeps no memory across
            # CLI runs); AGENTINO_GUI_REUSE_AUTH tells it to skip re-logging in a user whose
            # storageState phase 1 produced moments ago against this same running server.
            $env:AGENTINO_GUI_REUSE_AUTH = "1"
            & npx playwright test @PlaywrightArgs --grep '@mutating' --workers=1 | Out-Host
            $phase2 = $LASTEXITCODE
            Remove-Item Env:\AGENTINO_GUI_REUSE_AUTH -ErrorAction SilentlyContinue

            if ($phase1 -ne 0) { return $phase1 }
            return $phase2
        }
        finally {
            Remove-Item Env:\CI -ErrorAction SilentlyContinue
            Remove-Item Env:\AGENTINO_GUI_BASE_URL -ErrorAction SilentlyContinue
            Remove-Item Env:\AGENTINO_LISA_SERVICE_PATH -ErrorAction SilentlyContinue
            Remove-Item Env:\AGENTINO_PUMA_SERVICE_PATH -ErrorAction SilentlyContinue
            Remove-Item Env:\PLAYWRIGHT_OUTPUT_ROOT -ErrorAction SilentlyContinue
            Remove-Item Env:\PLAYWRIGHT_OUTPUT_PHASE -ErrorAction SilentlyContinue
        }
    }
    finally {
        $ErrorActionPreference = $previousEap
        Pop-Location
    }
}

Write-Step "Resolved paths"
Write-Host "RepoRoot:              $RepoRoot"
Write-Host "ScriptDir:             $ScriptDir"
Write-Host "AgentinoServerExePath: $AgentinoServerExePath"
Write-Host "AgentinoAgentExePath:  $AgentinoAgentExePath"
Write-Host "LisaServiceExePath:    $LisaServiceExePath"
Write-Host "PumaServiceExePath:    $PumaServiceExePath"
Write-Host "OutputRoot:            $OutputRoot"

try {
    Stop-ServerProcess "AgentinoServerTest"
    Stop-ServerProcess "AgentinoAgentTest"

    # After the processes are down (they hold the SQLite files open) and before anything starts again.
    Reset-AgentinoTestState

    Start-AgentinoTestServer
    Start-AgentinoTestAgent
    New-SuperuserIfNeeded
    Approve-TestAgent

    $exitCode = Invoke-PlaywrightSuite
}
finally {
    if ($serverProcess -and -not $serverProcess.HasExited) {
        Write-Step "Stopping AgentinoServerTest.exe (PID $($serverProcess.Id))"
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    }
    if ($agentProcess -and -not $agentProcess.HasExited) {
        Write-Step "Stopping AgentinoAgentTest.exe (PID $($agentProcess.Id))"
        Stop-Process -Id $agentProcess.Id -Force -ErrorAction SilentlyContinue
    }
}

if ($exitCode -eq 0) {
    Write-Host "`nAll tests passed." -ForegroundColor Green
} else {
    Write-Host "`nTest run failed (Playwright exit code $exitCode)." -ForegroundColor Red
}

exit $exitCode
