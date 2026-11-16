<#
.SYNOPSIS
    Runs the Agentino connection token API suite against the test server and agent.

.DESCRIPTION
    Starts AgentinoServerTest.exe and AgentinoAgentTest.exe from a clean test state with the functions
    of Tests\AgentinoGui\Run-CiTests.ps1 (the same ports, superuser and agent), then runs the Postman
    collection with newman. The production folders of Agentino are not touched.
#>
param(
    [string]$RepoRoot = $(if ($env:AGENTINODIR) { $env:AGENTINODIR } else { (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path }),
    [string]$BuildConfig = "Release_Qt6_VC17_x64",
    [int]$HttpPort = 17111,
    [int]$AgentGuiPort = 17222,
    [string]$SuPassword = "AgentinoGuiTest1!",
    [string]$AgentClientId = "a9e7c341-d5b4-4f28-9c61-0f17e5c0a901",
    [int]$StartupTimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
$SuiteDir = $PSScriptRoot
$ScriptDir = Join-Path $RepoRoot 'Tests\AgentinoGui'
$OutputRoot = Join-Path $SuiteDir 'test-output'
$AgentinoServerExePath = Join-Path $RepoRoot "Bin\$BuildConfig\AgentinoServerTest.exe"
$AgentinoAgentExePath = Join-Path $RepoRoot "Bin\$BuildConfig\AgentinoAgentTest.exe"
$serverProcess = $null
$agentProcess = $null

# The start of the test server and agent is the one of the GUI suite
$ast = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $ScriptDir 'Run-CiTests.ps1'), [ref]$null, [ref]$null)
foreach ($function in $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)) {
    Invoke-Expression $function.Extent.Text
}

$exitCode = 1
try {
    Stop-ServerProcess 'AgentinoServerTest'
    Stop-ServerProcess 'AgentinoAgentTest'
    New-Item -ItemType Directory -Force $OutputRoot | Out-Null
    Reset-AgentinoTestState
    Start-AgentinoTestServer
    Initialize-AgentinoTestAgentSettings
    Start-AgentinoTestAgent
    New-SuperuserIfNeeded
    Approve-TestAgent

    Write-Step 'Running the connection token suite'
    $newman = Join-Path $SuiteDir 'node_modules\.bin\newman.cmd'
    if (-not (Test-Path $newman)) {
        Push-Location $SuiteDir
        try { npm install --no-audit --no-fund | Out-Null } finally { Pop-Location }
    }

    & $newman run (Join-Path $SuiteDir 'Connection_Tokens.postman_collection.json') `
        -e (Join-Path $SuiteDir 'Connection_Tokens.postman_environment.json') `
        --env-var "base_url=http://localhost:$HttpPort/Agentino/graphql" `
        --env-var "su_password=$SuPassword" `
        --env-var "client_id=$AgentClientId" `
        --reporters 'cli,junit,json' `
        --reporter-junit-export (Join-Path $SuiteDir 'junit-report.xml') `
        --reporter-json-export (Join-Path $SuiteDir 'run-report.json')
    $exitCode = $LASTEXITCODE
}
finally {
    Stop-ServerProcess 'AgentinoServerTest'
    Stop-ServerProcess 'AgentinoAgentTest'
}

exit $exitCode
