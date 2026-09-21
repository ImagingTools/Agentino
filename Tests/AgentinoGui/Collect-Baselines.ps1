# SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-ImtCore-Commercial
<#
.SYNOPSIS
    Gathers the baselines a -UpdateSnapshots run actually changed, ready to be copied back.

.DESCRIPTION
    Screenshot baselines have to be minted on the machine that compares them, which is the build
    agent - see Run-CiTests.ps1's -UpdateSnapshots. This collects the result.

    Only what CHANGED, and under the same names and the same per-user folders the repository uses,
    so the artifact can be unzipped straight over tests/__screenshots__. Publishing the whole tree
    instead would be ~140 files with no indication of which ones moved.

    Deleted baselines cannot be copied, so they are listed in SUMMARY.txt instead - a baseline
    disappears when its test was renamed or removed, and the stale file then has to be deleted by
    hand.

    Exits 0 with an explanatory SUMMARY.txt when nothing changed, which is the normal case: every
    build runs this, only a -UpdateSnapshots one has anything to report.
#>
param(
    [Parameter(Mandatory = $true)][string]$RepoRoot,

    # Relative to $RepoRoot, e.g. "Tests/AgentinoGui/tests/__screenshots__".
    [Parameter(Mandatory = $true)][string]$BaselineDir,

    # Relative to $RepoRoot. Lives under test-output/, which is already gitignored.
    [Parameter(Mandatory = $true)][string]$OutputDir
)

$ErrorActionPreference = "Stop"

Push-Location $RepoRoot
try {
    $outFull = Join-Path $RepoRoot $OutputDir
    if (Test-Path $outFull) { Remove-Item $outFull -Recurse -Force }
    New-Item -ItemType Directory -Path $outFull -Force | Out-Null

    # --porcelain rather than a timestamp scan: -UpdateSnapshots rewrites every file, so mtimes move
    # even where the bytes are identical. Only git knows which images actually differ.
    $status = @(& git status --porcelain -- $BaselineDir)

    $copied = @()
    $deleted = @()
    foreach ($line in $status) {
        if ($line.Length -lt 4) { continue }
        $code = $line.Substring(0, 2).Trim()
        $path = $line.Substring(3).Trim().Trim('"')
        # A rename is reported as "old -> new"; only the new side is a file to carry back.
        if ($path -match ' -> ') { $path = ($path -split ' -> ')[-1] }

        $relative = $path.Substring($BaselineDir.Length).TrimStart('/', '\')
        if ($code -eq 'D') { $deleted += $relative; continue }

        $source = Join-Path $RepoRoot $path
        if (-not (Test-Path $source)) { continue }
        $target = Join-Path $outFull $relative
        New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
        Copy-Item $source $target -Force
        $copied += $relative
    }

    $lines = New-Object System.Collections.Generic.List[string]
    if ($copied.Count -eq 0 -and $deleted.Count -eq 0) {
        $lines.Add("No baseline changed in this run.")
        $lines.Add("")
        $lines.Add("Either the run did not set the 'Re-mint GUI screenshot baselines' parameter, or it did")
        $lines.Add("and every image came out byte-identical to the one already committed.")
    }
    else {
        $lines.Add("Baselines changed by this run, grouped by test user.")
        $lines.Add("")
        $lines.Add("Copy the folders below over Tests/<Suite>Gui/tests/__screenshots__ - the names and the")
        $lines.Add("layout are already the ones the repository uses.")
        $lines.Add("")

        foreach ($group in ($copied | Group-Object { ($_ -split '[\/]')[0] } | Sort-Object Name)) {
            $lines.Add("$($group.Name)  ($($group.Count) changed)")
            foreach ($f in ($group.Group | Sort-Object)) {
                $lines.Add("    " + ($f -split '[\/]', 2)[-1])
            }
            $lines.Add("")
        }

        if ($deleted.Count -gt 0) {
            $lines.Add("Gone - no test produces these any more, delete them by hand:")
            foreach ($f in ($deleted | Sort-Object)) { $lines.Add("    $f") }
            $lines.Add("")
        }
    }

    $summary = Join-Path $outFull "SUMMARY.txt"
    Set-Content -Path $summary -Value $lines -Encoding UTF8

    Write-Host ""
    foreach ($l in $lines) { Write-Host $l }
    Write-Host ""
    Write-Host "Collected $($copied.Count) changed baseline(s) into $OutputDir - published as gui-baselines.zip." -ForegroundColor Green
}
finally {
    Pop-Location
}
